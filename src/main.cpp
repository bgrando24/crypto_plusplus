#include <cpr/cpr.h>
#include <iostream>
#include "../include/websocket_client.h"
#include "../include/binance.h"
#include "simdjson.h"
#include "../include/circular_buffer.h"
#include "../include/order_book.h"
#include <thread>

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

    // create new order book
    OrderBook order_book("https://api.binance.com/api/v3/depth?symbol=XRPUSDT&limit=1024", buffer);
    // order_book.init();

    // Connect to Binance WebSocket API
    WebSocketClient client("stream.binance.com", 443, "/ws/xrpusdt@depth@100ms", binance_callback, &buffer);
    // client.init();

    // launch websocket client thread
    std::thread client_thread(&WebSocketClient::init, &client);
    // launch order ook thread
    std::thread order_book_thread(&OrderBook::init, &order_book);

    // wait for threads to complete
    client_thread.join();
    order_book_thread.join();

    return 0;
}
