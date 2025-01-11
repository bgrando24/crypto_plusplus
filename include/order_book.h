#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <unordered_map>
#include <queue>
#include <vector>
#include "circular_buffer.h"
#include "binance.h"
#include <cpr/cpr.h>
#include <iostream>
#include <thread>
#include "simdjson.h"

// https://developers.binance.com/docs/binance-spot-api-docs/web-socket-streams#diff-depth-stream

/**
 * The OrderBook class maintains a local order book data structure.
 * Bid/Ask price points and their quantities are stored in hash maps, and the max bid/min ask are stored in heaps.
 */
class OrderBook
{
private:
    // hashmaps for bid/ask prices and quantities - note Binance API returns quantity as floating point values

    // Bids: key = price, value = total order quantity
    std::unordered_map<double, double> bid_map;
    // Asks: key = price, value = total order quantity
    std::unordered_map<double, double> ask_map;

    // heaps for best bid/ask
    // Note: priority_queue is a container adopter - it uses vectors as its internal structure, and is why we pass a vector for the min heap

    // Stores best bid price (max)
    std::priority_queue<double> bid_heap;
    // Stores best ask price (min)
    std::priority_queue<double, std::vector<double>, std::greater<double>> ask_heap;

    // URL used for getting the order book snapshot
    std::string snapshot_url;
    // pointer to the data ingestion buffer
    CircularBuffer<Binance_DiffDepth, 1024> *data_buffer;

public:
    /**
     * Constructor
     * @param snapshot_url The Binance URL for obtaining a snapshot of the order book
     * @param data_buffer Pointer to the data ingestion buffer, where the WebSocket data is stored
     */
    OrderBook(std::string snapshot_url, CircularBuffer<Binance_DiffDepth, 1024> &data_buffer)
    {
        this->snapshot_url = snapshot_url;
        this->data_buffer = &data_buffer;
    }

    /**
     * Initialises the Order Book.
     * This involves validating the availability of the data buffer, obtaining the snapshot API response, and order book sychronisation
     * @returns true if initialisation successful, otherwise false
     * @throws std::invalid_argument If the data buffer is pointing to a nullptr, OR if the data buffer is flagged as not ready
     */
    bool init()
    {
        // Check if data buffer is valid
        if (this->data_buffer == nullptr)
        {
            throw std::invalid_argument("[OrderBook][init] Data buffer reference is pointing to nullptr!");
        }

        // wait for buffer to be ready
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "[OrderBook][init] Waiting for data buffer" << std::endl;
        } while (!this->data_buffer->get_is_ready());

        // get the first WebSocket event - we need the 'First update ID' of this event, as we use this to sync the snapshot
        Binance_DiffDepth first_update;
        bool read_success = false;
        int retry_count = 0;
        const int MAX_RETRIES = 10;
        const int MAX_SNAPSHOT_RETRIES = 10; // for the snapshot syncing

        // retry until buffer successfully read from, or limit hit
        while (!read_success && retry_count < MAX_RETRIES)
        {
            std::cout << "Attempt " << retry_count + 1 << " to read from buffer" << std::endl;
            std::cout << "Current buffer size: " << this->data_buffer->size() << std::endl;

            // try to read from the buffer
            read_success = this->data_buffer->try_read(first_update);

            if (read_success)
            {
                std::cout << "Successfully read from buffer:" << std::endl;
                std::cout << "Event: " << (first_update.event.empty() ? "EMPTY" : first_update.event) << std::endl;
                std::cout << "Event time: " << first_update.event_time << std::endl;
                std::cout << "First update ID: " << first_update.first_update_id << std::endl;
                std::cout << "Final update ID: " << first_update.final_update_id << std::endl;
            }
            else
            {
                std::cout << "Failed to read from buffer" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // iterate count
            retry_count++;
        }

        if (!read_success)
        {
            throw std::runtime_error("Failed to read initial update after maximum retries");
        }

        // now, get the snapshot - using CPR library for the HTTP request/response
        cpr::Response snapshot_response = cpr::Get(cpr::Url{this->snapshot_url});

        // check status code - expecting 200 if successful
        if (snapshot_response.status_code != 200)
        {
            throw std::runtime_error("[OrderBook][init] HTTP Response from Binance snapshot API returned non-200 status code");
        }

        // for testing, just check if we can receive the data correctly
        std::cout << "------------------- DEPTH SNAPSHOT -------------------" << std::endl;
        std::cout << snapshot_response.text << std::endl;

        // If the lastUpdateId from the snapshot is strictly less than the U from step 2, go back to step 3
        // check first update
        std::cout << "First update ID: " << first_update.first_update_id << std::endl;
        // check the lastUpdateId from the snapshot
        // Parse JSON payload
        simdjson::ondemand::parser parser;
        simdjson::ondemand::document doc = parser.iterate(snapshot_response.text);

        // Declare last_update_id variable
        std::string last_update_id;

        // Parse lastUpdateId
        simdjson::ondemand::value last_update_id_value = doc["lastUpdateId"];
        // Check if the value is a string, and if not, check for an integer
        if (last_update_id_value.is_string())
        {
            last_update_id = std::string(last_update_id_value.get_string().value());
        }
        else if (last_update_id_value.is_integer())
        {
            last_update_id = std::to_string(last_update_id_value.get_int64());
        }
        else if (last_update_id_value.is_null())
        {
            throw std::runtime_error("[OrderBook][init] lastUpdateId is missing or null, cannot proceed.");
        }
        else
        {
            throw std::runtime_error("[OrderBook][init] lastUpdateId has unexpected type");
        }

        std::cout << "Last update ID from snapshot: " << last_update_id << std::endl;

        // check if lastUpdateId < first_update_id, if so repeat fetching the snapshot until it is not
        int snapshot_retry_count = 0; // track how many retries
        std::cout << "Validating snapshot..." << std::endl;
        while (std::stol(last_update_id) < std::stol(first_update.first_update_id))
        {
            // stop if max retries hit
            if (snapshot_retry_count >= MAX_SNAPSHOT_RETRIES)
            {
                throw std::runtime_error("[OrderBook][init] Snapshot lastUpdateId < first update ID, maximum retries hit");
            }

            // fetch new snapshot
            std::cout << "[WARNING] Snapshot lastUpdateId < first update ID, fetching new snapshot" << std::endl;
            snapshot_response = cpr::Get(cpr::Url{this->snapshot_url});
            doc = parser.iterate(snapshot_response.text);
            last_update_id = std::string(doc["lastUpdateId"].get_string().value());
            std::cout << "Last update ID from snapshot: " << last_update_id << std::endl;

            // iterate count
            snapshot_retry_count++;
        }

        // snapshot now validated

        std::cout << "[OrderBook][init] Snapshot validated, parsing and storing bid/ask" << std::endl;

        // Parse bids array
        auto bids = doc["bids"];
        // check if bids is an array
        if (bids.type() == simdjson::ondemand::json_type::array)
        {
            // iterate over bids
            for (auto bid : bids.get_array())
            {
                auto bid_array = bid.get_array();

                // check if bid array has at least 2 elements, price and quantity
                if (bid_array.count_elements() >= 2)
                {
                    try
                    {
                        double price = std::stod(std::string(bid_array.at(0).get_string().value()));
                        double quantity = std::stod(std::string(bid_array.at(1).get_string().value()));

                        this->bid_map[price] = quantity;
                        this->bid_heap.push(price);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[OrderBook][init] Error parsing bid: " << e.what() << std::endl;
                    }
                }
                else
                {
                    std::cerr << "[OrderBook][init] Invalid bid array size" << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "[OrderBook][init] Bids data is not an array!" << std::endl;
        }

        // Parse asks array
        auto asks = doc["asks"];
        // check if asks is an array
        if (asks.type() == simdjson::ondemand::json_type::array)
        {
            // iterate over asks
            for (auto ask : asks.get_array())
            {
                auto ask_array = ask.get_array();

                // check if ask array has at least 2 elements, price and quantity
                if (ask_array.count_elements() >= 2)
                {
                    try
                    {
                        double price = std::stod(std::string(ask_array.at(0).get_string().value()));
                        double quantity = std::stod(std::string(ask_array.at(1).get_string().value()));

                        this->ask_map[price] = quantity;
                        this->ask_heap.push(price);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[OrderBook][init] Error parsing ask: " << e.what() << std::endl;
                    }
                }
                else
                {
                    std::cerr << "[OrderBook][init] Invalid ask array size" << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "[OrderBook][init] Asks data is not an array!" << std::endl;
        }

        std::cout << "[OrderBook][init] Snapshot validated and stored, checking buffered events" << std::endl;

        // in the buffered events (from data ingestion), we discard any event where its final update ID is less or equal to lastUpdateId
        // this should result 'lastUpdateId' within interval [U,u] (first update ID and final update ID from the first event)

        // iterate over buffer
        Binance_DiffDepth event; // we need somewhere to store the retrieved event from the buffer
        while (this->data_buffer->try_read(event))
        {
            // check if event is NOT within the interval
            if (std::stol(event.final_update_id) <= std::stol(last_update_id))
            {
                // remove from buffer
                this->data_buffer->try_pop(event);
            }
        }

        // now, we have the snapshot and the buffer is cleaned up - order book is ready to be continually synchronised

        return true;
    }

    /**
     * Synchronises the order book with the incoming WebSocket data - runs indefinitely
     * @returns true if synchronisation successful, otherwise false
     */
    bool keep_orderbook_sync()
    {
        // we check the data buffer continually, and update the order book with any new events
        Binance_DiffDepth event;
        while (true)
        {
            // check buffer is marked as ready (otherwise do nothing)
            if (this->data_buffer->get_is_ready())
            {
                // read and pop next event from buffer
                this->data_buffer->try_pop(event);

                // update order book
                // BIDS
                if (event.bids.size() > 0)
                {
                    // get price and quantity
                    double bid_price = std::stod(event.bids[0][0]);
                    double bid_quantity = std::stod(event.bids[0][1]);
                    // add bid price and quantity
                    this->bid_map[bid_price] = bid_quantity;
                    this->bid_heap.push(bid_price);
                }

                // ASKS
                if (event.asks.size() > 0)
                {
                    // get price and quantity
                    double ask_price = std::stod(event.asks[0][0]);
                    double ask_quantity = std::stod(event.asks[0][1]);
                    // add ask price and quantity
                    this->ask_map[ask_price] = ask_quantity;
                    this->ask_heap.push(ask_price);
                }
            }
        }
    }
};

#endif