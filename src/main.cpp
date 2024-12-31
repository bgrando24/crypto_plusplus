#include <cpr/cpr.h>
#include <iostream>
#include "../include/websocket_client.h"
#include "../include/binance.h"
#include "simdjson.h"
#include <queue>

/**
 * @brief Custom callback method for Binanec fstream websocket specifically
 * @param wsi The websocket instance
 * @param reason The reason for the callback
 * @param user User data
 * @param in Incoming data
 * @param len Length of incoming data
 * @param queue The queue to store incoming data
 */
int binance_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len, std::queue<Binance_AggTrade> &queue)
{
    // handle different Websocket events
    switch (reason)
    {
    // when WS connection established successfully
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
    {
        // Don't send message directly here!
        // Instead, request a callback when it's safe to write
        lws_callback_on_writable(wsi);
        break;
    }

    // when client is able to write
    case LWS_CALLBACK_CLIENT_WRITEABLE:
    {
        // Don't need to send anything to Binance server
        break;
    }

    // when client receives message from server
    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
        // print server message
        // std::cout << "Received from server: " << (char *)in << std::endl;

        // parse incoming JSON payload
        simdjson::ondemand::parser parser;
        simdjson::padded_string json_data((const char *)in, len);
        simdjson::ondemand::document doc;
        auto error = parser.iterate(json_data).get(doc);
        if (error)
        {
            std::cout << "Raw data: " << (char *)in << std::endl;
            std::cerr << "Error parsing JSON: " << error << std::endl;
            break;
        }

        // create binance struct from JSON payload
        Binance_AggTrade binance_aggTrade;
        try
        {
            // event type
            binance_aggTrade.event = std::string(doc["data"]["e"].get_string().value());
            // event time
            binance_aggTrade.event_time = doc["data"]["E"].get_int64().value();
            // symbol
            binance_aggTrade.symbol = std::string(doc["data"]["s"].get_string().value());
            // trade ID
            binance_aggTrade.trade_id = doc["data"]["a"].get_int64().value();

            // Parse price and quantity strings, then convert to double
            binance_aggTrade.price = std::stod(std::string(doc["data"]["p"].get_string().value()));
            binance_aggTrade.quantity = std::stod(std::string(doc["data"]["q"].get_string().value()));
            // first trade ID
            binance_aggTrade.first_trade_id = doc["data"]["f"].get_int64().value();
            // last trade ID
            binance_aggTrade.last_trade_id = doc["data"]["l"].get_int64().value();
            // trade time
            binance_aggTrade.trade_time = doc["data"]["T"].get_int64().value();
            // is buyer maker
            binance_aggTrade.is_buyer_maker = doc["data"]["m"].get_bool().value();
        }
        catch (const simdjson::simdjson_error &e)
        {
            std::cerr << "Error accessing JSON elements: " << e.what() << std::endl;
            break;
        }

        // print binance struct
        std::cout << "Event: " << binance_aggTrade.event << std::endl;
        std::cout << "Event time: " << binance_aggTrade.event_time << std::endl;
        std::cout << "Symbol: " << binance_aggTrade.symbol << std::endl;
        std::cout << "Trade ID: " << binance_aggTrade.trade_id << std::endl;
        std::cout << "Price: " << binance_aggTrade.price << std::endl;
        std::cout << "Quantity: " << binance_aggTrade.quantity << std::endl;
        std::cout << "First trade ID: " << binance_aggTrade.first_trade_id << std::endl;
        std::cout << "Last trade ID: " << binance_aggTrade.last_trade_id << std::endl;
        std::cout << "Trade time: " << binance_aggTrade.trade_time << std::endl;
        std::cout << "Is buyer maker: " << binance_aggTrade.is_buyer_maker << std::endl;
        std::cout << std::endl;

        // push binance struct to queue
        queue.push(binance_aggTrade);
        break;
    }

    // when the connection is closed, by either client or server
    case LWS_CALLBACK_CLIENT_CLOSED:
    {
        std::cout << "Connection to server closed" << std::endl;
        break;
    }

    default:
    {
        // do nothing
        break;
    }
    }

    // return 0 to indicate success
    return 0;
};

int main(int argc, char **argv)
{
    // ------------------------------------ WebSockets with libwebsockets ------------------------------------

    // connect to binance WS API
    WebSocketClient client = WebSocketClient("fstream.binance.com", 443, "stream?streams=bnbusdt@aggTrade/btcusdt@markPrice", binance_callback);
    client.init();
}