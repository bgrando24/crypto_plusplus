// implementation for WebSocketClient class

#include <libwebsockets.h>
#include <iostream>
#include <thread>
#include "../include/websocket_client.h"

/**
 * @brief Default constructor, uses static default_callback
 * @param uri The WS server URI
 * @param port The WS server port
 */
WebSocketClient::WebSocketClient(char *uri, int port)
{
    this->uri = uri;
    this->port = port;
    this->path = "/";
    // use default_callback
    this->callback = WebSocketClient::default_callback;
};

/**
 * @brief Constructor that allows for specifying a path for the host
 * @param uri The WS server URI
 * @param port The WS server port
 * @param path The WS server path
 */
WebSocketClient::WebSocketClient(char *uri, int port, char *path)
{
    this->uri = uri;
    this->port = port;
    this->path = path;
    // use default_callback
    this->callback = WebSocketClient::default_callback;
};

/**
 * @brief Constructor allowing all parameters to be specified
 * @param uri The WS server URI
 * @param port The WS server port
 * @param path The WS server path
 * @param callback The custom callback method
 */
WebSocketClient::WebSocketClient(
    const char *uri,
    int port,
    const char *path,
    int (*callback)(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len),
    CircularBuffer<Binance_AggTrade, 8> *buffer)
{
    this->uri = uri;
    this->port = port;
    this->path = path;
    this->callback = callback;
    this->buffer = buffer;

    if (this->buffer == nullptr)
    {
        std::cerr << "Error: Buffer is not initialized!" << std::endl;
    }
    else
    {
        std::cout << "Buffer initialized at address: " << this->buffer << std::endl;
    }
}

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
    // Create client data structure
    WebSocketClientData *client_data = new WebSocketClientData;
    client_data->buffer = this->buffer;
    client_data->client = this;

    // define WS client protocol
    static struct lws_protocols protocols[] = {
        {
            "my-protocol",
            this->callback,
            sizeof(WebSocketClientData), // use our data structure size,
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
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT; // SSL global init

    // create WS context, this holds the state of the WS connection - pass in mem addr of into struct
    struct lws_context *context = lws_create_context(&info);
    if (!context)
    {
        std::cerr << "Context creation failed" << std::endl;
        return -1;
    }

    // set logging level
    // lws_set_log_level(LLL_DEBUG | LLL_INFO | LLL_WARN | LLL_ERR, NULL);
    lws_set_log_level(LLL_WARN | LLL_ERR, NULL);

    // set up WS client connection info
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));

    ccinfo.context = context;
    ccinfo.address = this->uri;
    ccinfo.port = this->port;
    ccinfo.path = this->path;
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK; // Use SSL/TLS for the connection
    ccinfo.alpn = "http/1.1";                                                        // Application Layer Protocol Negotiation
    ccinfo.userdata = client_data;

    std::cout << "Connecting to " << ccinfo.address << ":" << ccinfo.port << ccinfo.path << std::endl;

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
    while (lws_service(context, 0) >= 0)
    {
        // small sleep to prevent CPU spinning
        lws_callback_on_writable(wsi);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // clean up
    lws_context_destroy(context);
    return 0;
};

// Get buffer instance
CircularBuffer<Binance_AggTrade, 8> *WebSocketClient::get_buffer()
{
    if (!this->buffer)
    {
        std::cerr << "Error: Buffer is null" << std::endl;
        throw std::runtime_error("Buffer is not initialized");
    }
    std::cout << "Returning buffer at address: " << this->buffer << std::endl;
    return this->buffer;
}