// Header file for websocket.cpp - wrapper around libwebsockets library
#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <libwebsockets.h>
#include <iostream>
#include <queue>
#include "binance.h"

/**
 * @brief The WebSocketClient class is a wrapper around the libwebsockets library. It enables connection to a websocket server as a client
 */
class WebSocketClient
{
private:
    // libwebsocket context
    struct lws_context *context;
    // libwebsocket instance
    struct lws *wsi;
    // libwebsocket info
    struct lws_client_connect_info ccinfo;
    // WS server URI
    const char *uri;
    // WS server path
    const char *path;
    // WS server port
    int port;
    // callback method, uses default_callback if custom callback not provided
    int (*callback)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
    // callback method that also takes in a queue
    int (*callback_queue)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len, std::queue<Binance_AggTrade> &queue);

public:
    // constructors

    /**
     * @brief Default constructor, uses static default_callback
     * @param uri The WS server URI
     * @param port The WS server port
     */
    WebSocketClient(char *uri, int port);
    /**
     * @brief Constructor that allows for specifying a path for the host
     * @param uri The WS server URI
     * @param port The WS server port
     * @param path The WS server path
     */
    WebSocketClient(char *uri, int port, char *path);
    /**
     * @brief Constructor allowing all parameters to be specified
     * @param uri The WS server URI
     * @param port The WS server port
     * @param path The WS server path
     * @param callback The custom callback method
     */
    WebSocketClient(char *uri, int port, char *path, int (*callback)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len));

    /**
     * @brief Constructor allowing all parameters to be specified, including a queue
     * @param uri The WS server URI
     * @param port The WS server port
     * @param path The WS server path
     * @param callback The custom callback method
     * @param queue The queue to store incoming data
     */
    WebSocketClient(char *uri, int port, char *path, int (*callback)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len, std::queue<Binance_AggTrade> &queue));

    // default callback method
    static int default_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

    // initialise Websocket connection
    int init();
};

#endif