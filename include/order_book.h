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

    /**
     * @brief Parses a price level from a JSON array.
     *
     * This method extracts the price and quantity from a JSON array representing a price level.
     * The JSON array is expected to have at least two elements: the first element is the price as a string,
     * and the second element is the quantity as a string. These values are converted to double and stored
     * in the provided references.
     *
     * @param level The JSON array representing the price level.
     * @param price Reference to a double where the parsed price will be stored.
     * @param quantity Reference to a double where the parsed quantity will be stored.
     * @return true if the price level was successfully parsed, false otherwise.
     */
    bool parse_price_level(simdjson::ondemand::array level, double &price, double &quantity)
    {
        try
        {
            // Get values directly using at() to avoid iteration
            auto price_val = level.at(0);
            auto qty_val = level.at(1);

            std::string price_str, quantity_str;

            // Handle price
            if (price_val.type() == simdjson::ondemand::json_type::string)
            {
                price_str = std::string(price_val.get_string().value());
            }
            else if (price_val.type() == simdjson::ondemand::json_type::number)
            {
                price_str = std::to_string(price_val.get_double().value());
            }
            else
            {
                return false;
            }

            // Handle quantity
            if (qty_val.type() == simdjson::ondemand::json_type::string)
            {
                quantity_str = std::string(qty_val.get_string().value());
            }
            else if (qty_val.type() == simdjson::ondemand::json_type::number)
            {
                quantity_str = std::to_string(qty_val.get_double().value());
            }
            else
            {
                return false;
            }

            price = std::stod(price_str);
            quantity = std::stod(quantity_str);
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error parsing price level: " << e.what() << std::endl;
            return false;
        }
    }

public:
    /**
     * @brief Construct a new OrderBook object
     *
     * @param snapshot_url The URL to fetch the order book snapshot from
     * @param data_buffer Reference to the data buffer for order book updates
     */
    OrderBook(std::string snapshot_url, CircularBuffer<Binance_DiffDepth, 1024> &data_buffer)
        : snapshot_url(snapshot_url), data_buffer(&data_buffer) {}

    /**
     * Initialises the Order Book.
     * This involves validating the availability of the data buffer, obtaining the snapshot API response, and order book sychronisation
     * @returns true if initialisation successful, otherwise false
     * @throws std::invalid_argument If the data buffer is pointing to a nullptr, OR if the data buffer is flagged as not ready
     */
    bool init()
    {
        // if data buffer reference is null, throw an exception
        if (this->data_buffer == nullptr)
        {
            throw std::invalid_argument("[OrderBook][init] Data buffer reference is pointing to nullptr!");
        }

        // wait for data buffer to be ready
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "[OrderBook][init] Waiting for data buffer" << std::endl;
        } while (!this->data_buffer->get_is_ready());

        // read the first update from the buffer
        Binance_DiffDepth first_update;
        bool read_success = false;
        const int MAX_RETRIES = 10;
        const int MAX_SNAPSHOT_RETRIES = 10;

        // try to read the first update from the buffer
        for (int retry_count = 0; retry_count < MAX_RETRIES && !read_success; retry_count++)
        {
            std::cout << "Attempt " << retry_count + 1 << " to read from buffer" << std::endl;
            std::cout << "Current buffer size: " << this->data_buffer->size() << std::endl;

            read_success = this->data_buffer->try_read(first_update);

            if (read_success)
            {
                std::cout << "Successfully read from buffer:" << std::endl;
                std::cout << "First update ID: " << first_update.first_update_id << std::endl;
            }
            else
            {
                std::cout << "Failed to read from buffer" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // if we failed to read the first update after maximum retries, throw an exception
        if (!read_success)
        {
            throw std::runtime_error("[OrderBook][init] Failed to read initial update after maximum retries");
        }

        // parse the first update ID
        int64_t first_update_id;
        try
        {
            first_update_id = std::stoll(first_update.first_update_id);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("[OrderBook][init] Failed to parse first_update_id: " + std::string(e.what()));
        }

        // create a JSON parser
        simdjson::ondemand::parser parser;
        int64_t last_update_id = 0;

        // fetch the snapshot and parse it
        for (int snapshot_retry_count = 0; snapshot_retry_count < MAX_SNAPSHOT_RETRIES; snapshot_retry_count++)
        {
            cpr::Response snapshot_response = cpr::Get(cpr::Url{this->snapshot_url});
            if (snapshot_response.status_code != 200)
            {
                std::cerr << "HTTP error: " << snapshot_response.status_code << std::endl;
                continue;
            }

            // parse the snapshot - if the last update ID (from snapshot) is greater than or equal to the first update ID (from first event in buffer), we have a valid snapshot
            try
            {
                auto doc = parser.iterate(snapshot_response.text);
                last_update_id = doc["lastUpdateId"].get_int64();

                if (last_update_id >= first_update_id)
                {
                    std::cout << "Last update ID from snapshot: " << last_update_id << std::endl;

                    // Parse bids with more careful array handling
                    auto bids = doc["bids"].get_array();
                    for (auto bid : bids)
                    {
                        double price, quantity;
                        if (parse_price_level(bid.get_array(), price, quantity))
                        {
                            if (quantity > 0)
                            {
                                bid_map[price] = quantity;
                                bid_heap.push(price);
                            }
                        }
                    }

                    // Parse asks with more careful array handling
                    auto asks = doc["asks"].get_array();
                    for (auto ask : asks)
                    {
                        double price, quantity;
                        if (parse_price_level(ask.get_array(), price, quantity))
                        {
                            if (quantity > 0)
                            {
                                ask_map[price] = quantity;
                                ask_heap.push(price);
                            }
                        }
                    }
                    break;
                }
            }
            catch (const simdjson::simdjson_error &e)
            {
                std::cerr << "[OrderBook][init] JSON parsing error: " << e.what() << std::endl;
                continue;
            }

            // if the last update ID is invalid, wait and retry
            std::cout << "[WARNING] Snapshot lastUpdateId < first update ID, fetching new snapshot" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // if we failed to get a valid snapshot after maximum retries, throw an exception
        if (last_update_id < first_update_id)
        {
            throw std::runtime_error("[OrderBook][init] Failed to get valid snapshot after maximum retries");
        }

        std::cout << "[OrderBook][init] Snapshot validated and stored, checking buffered events" << std::endl;

        // Clean up buffer - remove events with final update ID <= last_update_id
        Binance_DiffDepth event;
        while (this->data_buffer->try_pop(event)) // Use try_pop instead of try_read
        {
            try
            {
                if (event.final_update_id.empty())
                {
                    std::cerr << "Warning: Empty final_update_id encountered" << std::endl;
                    continue; // Skip this event
                }

                int64_t event_final_id = std::stoll(event.final_update_id);

                // If we find an event with final_update_id > last_update_id,
                // we need to put it back in the buffer as it's still needed
                if (event_final_id > last_update_id)
                {
                    if (!this->data_buffer->try_push(event))
                    {
                        std::cerr << "Failed to push back valid event to buffer" << std::endl;
                    }
                    break; // Exit the loop as all subsequent events will also be newer
                }

                // Otherwise, the event is old and has been removed by try_pop
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error processing buffered event: " << e.what() << " (final_update_id: '" << event.final_update_id << "')" << std::endl;
            }
        }

        // order book is synced
        std::cout << "[OrderBook][init] Order book is synced!" << std::endl;
        return true;
    }

    /**
     * @brief Continuously update the order book using the websocket data buffer events
     */
    bool keep_orderbook_sync()
    {
        // stores the event to be processed
        Binance_DiffDepth event;
        int64_t local_update_id = 0; // Local order book update ID

        // continuously process events from the buffer
        while (true)
        {
            // check if the buffer is ready and try to pop an event
            if (this->data_buffer->get_is_ready() && this->data_buffer->try_pop(event))
            {
                try
                {
                    int64_t event_first_update_id = std::stoll(event.first_update_id);
                    int64_t event_last_update_id = std::stoll(event.final_update_id);

                    // If the event's last update ID is less than the local update ID, ignore the event
                    if (event_last_update_id < local_update_id)
                    {
                        continue;
                    }

                    // If the event's first update ID is greater than the local update ID, discard the local order book and restart
                    if (event_first_update_id > local_update_id)
                    {
                        std::cerr << "Event's first update ID is greater than the local update ID. Discarding local order book and restarting." << std::endl;

                        // Call init() to reinitialize the order book
                        if (!this->init())
                        {
                            std::cerr << "Failed to re-initialize the order book." << std::endl;
                            return false; // Return false to indicate failure to re-initialize
                        }

                        // Reset the local update ID after re-initialization
                        local_update_id = event_last_update_id;
                        std::cout << "Order book re-initialized successfully." << std::endl;
                    }

                    // Update bids and asks
                    for (const auto &bid : event.bids)
                    {
                        if (bid.size() >= 2)
                        {
                            double price = std::stod(bid[0]);
                            double quantity = std::stod(bid[1]);

                            if (quantity > 0)
                            {
                                bid_map[price] = quantity;
                                bid_heap.push(price);
                            }
                            else
                            {
                                bid_map.erase(price);
                            }
                        }
                    }

                    for (const auto &ask : event.asks)
                    {
                        if (ask.size() >= 2)
                        {
                            double price = std::stod(ask[0]);
                            double quantity = std::stod(ask[1]);

                            if (quantity > 0)
                            {
                                ask_map[price] = quantity;
                                ask_heap.push(price);
                            }
                            else
                            {
                                ask_map.erase(price);
                            }
                        }
                    }

                    // Set the local update ID to the event's last update ID
                    local_update_id = event_last_update_id;

                    // Log that the update was processed
                    std::cout << "Processed update: " << event.final_update_id << std::endl;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error processing update: " << e.what() << std::endl;
                }
            }

            // check if the top of each heap is still a valid price in the maps, otherwise remove value
            while (!bid_heap.empty() && bid_map.find(bid_heap.top()) == bid_map.end())
            {
                bid_heap.pop();
            }

            while (!ask_heap.empty() && ask_map.find(ask_heap.top()) == ask_map.end())
            {
                ask_heap.pop();
            }

            // log best current bid/ask
            if (!bid_heap.empty() && !ask_heap.empty())
            {
                std::cout << "Best bid: $" << bid_heap.top() << " Best ask: $" << ask_heap.top() << std::endl;
                double spread = ask_heap.top() - bid_heap.top();
                std::cout << "Spread: $" << spread << std::endl;
            }

            // sleep for a short time before checking the buffer again
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

#endif