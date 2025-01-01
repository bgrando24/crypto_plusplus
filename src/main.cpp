#include <cpr/cpr.h>
#include <iostream>
#include "../include/websocket_client.h"
#include "../include/binance.h"
#include "simdjson.h"
#include "../include/circular_buffer.h"

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

    CircularBuffer<Binance_AggTrade, 8> *buffer = client_data->buffer; // Access the buffer through the client

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
        simdjson::ondemand::document doc;
        auto error = parser.iterate(json_data).get(doc);
        if (error)
        {
            std::cerr << "Error parsing JSON: " << error << std::endl;
            break;
        }

        // Create Binance struct from JSON payload
        Binance_AggTrade binance_aggTrade;
        try
        {
            binance_aggTrade.event = std::string(doc["data"]["e"].get_string().value());
            binance_aggTrade.event_time = doc["data"]["E"].get_int64().value();
            binance_aggTrade.symbol = std::string(doc["data"]["s"].get_string().value());
            binance_aggTrade.trade_id = doc["data"]["a"].get_int64().value();
            binance_aggTrade.price = std::stod(std::string(doc["data"]["p"].get_string().value()));
            binance_aggTrade.quantity = std::stod(std::string(doc["data"]["q"].get_string().value()));
            binance_aggTrade.first_trade_id = doc["data"]["f"].get_int64().value();
            binance_aggTrade.last_trade_id = doc["data"]["l"].get_int64().value();
            binance_aggTrade.trade_time = doc["data"]["T"].get_int64().value();
            binance_aggTrade.is_buyer_maker = doc["data"]["m"].get_bool().value();
        }
        catch (const simdjson::simdjson_error &e)
        {
            std::cerr << "Error accessing JSON elements: " << e.what() << std::endl;
            break;
        }

        // Print Binance struct
        std::cout << "Event: " << binance_aggTrade.event << "\n"
                  << "Event time: " << binance_aggTrade.event_time << "\n"
                  << "Symbol: " << binance_aggTrade.symbol << "\n"
                  << "Trade ID: " << binance_aggTrade.trade_id << "\n"
                  << "Price: " << binance_aggTrade.price << "\n"
                  << "Quantity: " << binance_aggTrade.quantity << "\n"
                  << "First trade ID: " << binance_aggTrade.first_trade_id << "\n"
                  << "Last trade ID: " << binance_aggTrade.last_trade_id << "\n"
                  << "Trade time: " << binance_aggTrade.trade_time << "\n"
                  << "Is buyer maker: " << binance_aggTrade.is_buyer_maker << "\n"
                  << std::endl;

        // Push Binance struct to circular buffer
        if (!buffer->try_push(binance_aggTrade))
        {
            std::cerr << "Failed to push to buffer" << std::endl;
        }
        else
        {
            std::cout << "Successfully pushed to buffer" << std::endl;
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
    // Create a buffer
    CircularBuffer<Binance_AggTrade, 8> buffer;

    // Connect to Binance WebSocket API
    WebSocketClient client("fstream.binance.com", 443, "stream?streams=bnbusdt@aggTrade/btcusdt@markPrice", binance_callback, &buffer);
    client.init();

    return 0;
}
