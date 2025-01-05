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

        // check if data buffer is ready
        // if (!this->data_buffer->get_is_ready())
        // {
        //     throw std::invalid_argument("[OrderBook][init] Data buffer is flagged as NOT ready!");
        // }

        // wait for buffer to be ready
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "[OrderBook][init] Waiting for data buffer" << std::endl;

        } while (!this->data_buffer->get_is_ready());

        // get the first WebSocket event - we need the 'First update ID' of this event, as we use this to sync the snapshot
        Binance_DiffDepth first_update;
        // note we use try_read as we don't yet want to pop the event from the buffer
        this->data_buffer->try_read(first_update);

        // now, get the snapshot - using CPR library for the HTTP request/response
        cpr::Response snapshot_response = cpr::Get(cpr::Url{this->snapshot_url});

        // check status code - expecting 200 if successful
        if (snapshot_response.status_code != 200)
        {
            throw std::runtime_error("[OrderBook][init] HTTP Response from Binance snapshot API returned non-200 status code");
        }

        // for testing, just check if we can receive the data correctly
        std::cout << "------------------- INIT - JSON -------------------" << std::endl;
        std::cout << snapshot_response.text << std::endl;

        // NEXT: If the lastUpdateId from the snapshot is strictly less than the U from step 2, go back to step 3
        // Need to parse JSON response properly, in order to store values in hashmaps
        return true;
    }
};

#endif