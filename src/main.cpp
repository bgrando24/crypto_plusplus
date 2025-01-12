#include <cpr/cpr.h>
#include <iostream>
#include "../include/websocket_client.h"
#include "../include/binance.h"
#include "simdjson.h"
#include "../include/circular_buffer.h"
#include "../include/order_book.h"
#include <thread>

// Helper function to safely parse bid/ask arrays
bool parse_order_array(simdjson::ondemand::array array, std::vector<std::array<std::string, 2>> &orders)
{
    try
    {
        // Pre-allocate to avoid reallocation
        orders.clear();
        orders.reserve(1024); // reasonable default size for depth updates

        // Single pass through the array
        for (auto element : array)
        {
            auto values = element.get_array();
            std::array<std::string, 2> entry;

            // Get first value (price)
            auto price_val = values.at(0);
            if (price_val.type() == simdjson::ondemand::json_type::string)
            {
                entry[0] = std::string(price_val.get_string().value());
            }
            else if (price_val.type() == simdjson::ondemand::json_type::number)
            {
                entry[0] = std::to_string(price_val.get_double().value());
            }
            else
            {
                continue; // Skip invalid entries
            }

            // Get second value (quantity)
            auto qty_val = values.at(1);
            if (qty_val.type() == simdjson::ondemand::json_type::string)
            {
                entry[1] = std::string(qty_val.get_string().value());
            }
            else if (qty_val.type() == simdjson::ondemand::json_type::number)
            {
                entry[1] = std::to_string(qty_val.get_double().value());
            }
            else
            {
                continue; // Skip invalid entries
            }

            orders.push_back(entry);
        }
        return true;
    }
    catch (const simdjson::simdjson_error &e)
    {
        std::cerr << "Error parsing order array: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Custom callback method for Binance fstream websocket specifically
 * @param wsi The websocket instance
 * @param reason The reason for the callback
 * @param user User data
 * @param in Incoming data
 * @param len Length of incoming data
 */
int binance_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    WebSocketClientData *client_data = static_cast<WebSocketClientData *>(user);

    // Only proceed if we have valid client data
    if (!client_data)
    {
        return 0;
    }

    CircularBuffer<Binance_DiffDepth, 1024> *buffer = client_data->buffer; // Access the buffer through the client

    // Handle different WebSocket events
    switch (reason)
    {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        // No action needed
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
        std::cout << "--------------- Buffer size: " << buffer->size() << " ---------------" << std::endl;

        // Parse incoming JSON payload
        simdjson::ondemand::parser parser;
        simdjson::padded_string json_data((const char *)in, len);

        try
        {
            auto doc = parser.iterate(json_data);

            // Create Binance struct from JSON payload
            Binance_DiffDepth event_update;

            // Parse basic fields
            event_update.event = std::string(doc["e"].get_string().value());
            event_update.event_time = doc["E"].get_int64();
            event_update.symbol = std::string(doc["s"].get_string().value());
            event_update.first_update_id = std::to_string(doc["U"].get_int64());
            event_update.final_update_id = std::to_string(doc["u"].get_int64());

            // Parse bids array
            auto bids = doc["b"].get_array();
            for (auto bid : bids)
            {
                std::array<std::string, 2> bid_entry;
                auto bid_array = bid.get_array();
                size_t index = 0;
                for (auto value : bid_array)
                {
                    if (index < 2)
                    {
                        bid_entry[index] = std::string(value.get_string().value());
                        index++;
                    }
                }
                event_update.bids.push_back(bid_entry);
            }

            // Parse asks array
            auto asks = doc["a"].get_array();
            for (auto ask : asks)
            {
                std::array<std::string, 2> ask_entry;
                auto ask_array = ask.get_array();
                size_t index = 0;
                for (auto value : ask_array)
                {
                    if (index < 2)
                    {
                        ask_entry[index] = std::string(value.get_string().value());
                        index++;
                    }
                }
                event_update.asks.push_back(ask_entry);
            }

            // Push to buffer
            if (!buffer->try_push(event_update))
            {
                std::cerr << "Failed to push to buffer" << std::endl;
            }
            else
            {
                std::cout << "Successfully pushed to buffer" << std::endl;
            }
        }
        catch (const simdjson::simdjson_error &e)
        {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_CLOSED:
        std::cout << "Connection to server closed" << std::endl;
        // Clean up our client data when connection closes
        if (client_data)
        {
            delete client_data;
        }
        break;

    default:
        break;
    }

    return 0; // Indicate success
}

int main(int argc, char **argv)
{
    // Create a buffer for data ingestion
    CircularBuffer<Binance_DiffDepth, 1024> buffer;

    // Create new order book
    OrderBook order_book("https://api.binance.com/api/v3/depth?symbol=XRPUSDT&limit=1024", buffer);

    // Connect to Binance WebSocket API
    WebSocketClient client("stream.binance.com", 443, "/ws/xrpusdt@depth@100ms", binance_callback, &buffer);

    // Used to track if the init method for the order book is complete
    std::atomic<bool> order_book_init_done(false);

    // Launch websocket client thread
    std::thread client_thread(&WebSocketClient::init, &client);

    // Launch order book thread - for init
    std::thread order_book_init_thread([&order_book, &order_book_init_done]()
                                       {
        order_book.init();
        order_book_init_done.store(true, std::memory_order_release); });

    // Wait for order book initialization to complete
    while (!order_book_init_done.load(std::memory_order_acquire))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Start the sync thread
    std::thread order_book_sync_thread(&OrderBook::keep_orderbook_sync, &order_book);

    // Wait for all threads
    client_thread.join();
    order_book_init_thread.join();
    order_book_sync_thread.join(); // Add this to keep the program running

    return 0;
}
