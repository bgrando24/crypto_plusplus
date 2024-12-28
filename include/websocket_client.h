// Header file for websocket.cpp - wrapper around libwebsockets library
#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <libwebsockets.h>
#include <iostream>

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
    // WS server port
    int port;
    // callback method, uses default_callback if custom callback not provided
    int (*callback)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

public:
    // constructors

    /**
     * @brief Default constructor, uses static default_callback
     * @param uri The WS server URI
     * @param port The WS server port
     */
    WebSocketClient(char *uri, int port);
    /**
     * @brief Constructor with custom callback
     * @param uri The WS server URI
     * @param callback The custom callback method
     */
    WebSocketClient(char *uri, int (*callback)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len));

    // default callback method
    static int default_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

    // initialise Websocket connection
    int init();
};

#endif