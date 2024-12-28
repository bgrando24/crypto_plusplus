// implementation for WebSocketClient class

#include <libwebsockets.h>
#include <iostream>
#include <thread>
#include "../include/websocket_client.h"

/**
 * @brief Default constructor, uses static default_callback
 * @param uri The WS server URI
 */
WebSocketClient::WebSocketClient(char *uri, int port)
{
    this->uri = uri;
    this->port = port;
    // use default_callback
    this->callback = WebSocketClient::default_callback;
};

/**
 * @brief Constructor with custom callback
 * @param uri The WS server URI
 * @param callback The custom callback method
 */
WebSocketClient::WebSocketClient(char *uri, int (*callback)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len))
{
    this->uri = uri;
    this->callback = callback;
};

/**
 * @brief Default callback method for handling different Websocket events
 * @param wsi The websocket instance
 * @param reason The reason for the callback
 * @param user User data
 * @param in Incoming data
 * @param len Length of incoming data
 */
int WebSocketClient::default_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
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
        // Now it's safe to write
        const char *msg = "Hello from client!";
        unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512];
        memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], msg, strlen(msg));

        int written = lws_write(wsi,
                                &buf[LWS_SEND_BUFFER_PRE_PADDING],
                                strlen(msg),
                                LWS_WRITE_TEXT);

        if (written < 0)
        {
            std::cerr << "Error writing to socket" << std::endl;
            return -1;
        }
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

/**
 * @brief Initialise Websocket connection
 */
int WebSocketClient::init()
{
    // define WS client protocol
    static struct lws_protocols protocols[] = {
        {
            "my-protocol",
            this->callback,
            0,
            1024,
        },
        {NULL, NULL, 0, 0}};

    // create WS client
    struct lws_context_creation_info info;
    // initialise struct with zeros
    memset(&info, 0, sizeof(info));

    // we don't want to listen for incoming connections
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    // create WS context, this holds the state of the WS connection - pass in mem addr of into struct
    struct lws_context *context = lws_create_context(&info);
    if (!context)
    {
        std::cerr << "Context creation failed" << std::endl;
        return -1;
    }

    // set up WS client connection info
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));

    ccinfo.context = context;
    ccinfo.address = this->uri;
    ccinfo.port = this->port;
    ccinfo.path = "/";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0;

    std::cout << "Connecting to " << ccinfo.address << ":" << ccinfo.port << std::endl;

    // establish websocket connection to server
    struct lws *wsi = lws_client_connect_via_info(&ccinfo);

    // check if connection was successful
    if (wsi == NULL)
    {
        std::cerr << "Client connection failed" << std::endl;
        lws_context_destroy(context);
        return -1;
    }

    // Event loop
    while (lws_service(context, 50) >= 0)
    {
        // Added a small sleep to prevent CPU spinning
        lws_callback_on_writable(wsi);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // clean up
    lws_context_destroy(context);
    return 0;
};