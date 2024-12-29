#include <cpr/cpr.h>
#include <iostream>
#include "../include/websocket_client.h"

/**
 * @brief Custom callback method for Binanec fstream websocket specifically
 * @param wsi The websocket instance
 * @param reason The reason for the callback
 * @param user User data
 * @param in Incoming data
 * @param len Length of incoming data
 */
int binance_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
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
        std::cout << "Received from server: " << (char *)in << std::endl;
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
    // ------------------------------------ HTTPS with CPR ------------------------------------
    // // Fetch the content at https://api.github.com/repos/whoshuu/cpr/contributors
    // // and capture result in object 'r'
    // cpr::Response r = cpr::Get(
    //     cpr::Url{"https://api.github.com/repos/whoshuu/cpr/contributors"},
    //     cpr::Authentication("user", "pass", cpr::AuthMode::BASIC),
    //     cpr::Parameters{{"anon", "true"}, {"key", "value"}});

    // // print result items

    // std::cout << r.status_code << std::endl;
    // std::cout << r.header["content-type"] << std::endl;
    // std::cout << r.text << std::endl;
    // return 0;

    // ------------------------------------ WebSockets with libwebsockets ------------------------------------
    // create WS client
    // WebSocketClient client = WebSocketClient("127.0.0.1", 7681);
    // // initialise WS client
    // client.init();

    // connect to binance WS API
    WebSocketClient client = WebSocketClient("fstream.binance.com", 443, "stream?streams=bnbusdt@aggTrade/btcusdt@markPrice", binance_callback);
    client.init();
}