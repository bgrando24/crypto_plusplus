// filepath: /Users/brandongrando/Documents/High_Performance_C++/crypto_plusplus/include/binance.h
#ifndef BINANCE_H
#define BINANCE_H

#include <string>
#include <vector>
#include <stdexcept>

enum class CryptoSymbol
{
    BTC,
    ETH,
    LTC,
    XRP,
    // Add more symbols as needed
};

// Aggregate trade stream JSON payload: https://developers.binance.com/docs/derivatives/usds-margined-futures/websocket-market-streams/Aggregate-Trade-Streams
struct Binance_AggTrade
{
    std::string event;   // Event type
    long event_time;     // Event time
    std::string symbol;  // Symbol
    int trade_id;        // Trade ID
    double price;        // Trade price
    double quantity;     // Trade quantity
    long first_trade_id; // First trade ID
    long last_trade_id;  // Last trade ID
    long trade_time;     // Trade time
    bool is_buyer_maker; // Buyer is maker
};

// Spot Trade order book price/quantity depth update JSON payload: https://developers.binance.com/docs/binance-spot-api-docs/web-socket-streams#diff-depth-stream
struct Binance_DiffDepth
{
    std::string event;           // event type
    long event_time;             // event time
    std::string symbol;          // symbol
    std::string first_update_id; // first update ID in event
    std::string final_update_id; // final update ID in event
    // Each payload has bids/asks in arrays of varying length - each bid/ask has two elements: price and quantity
    std::vector<std::array<std::string, 2>> bids; // bids to be updated
    std::vector<std::array<std::string, 2>> asks; // asks to be updated
};

// Helper functions to convert CryptoSymbol to/from string
std::string to_string(CryptoSymbol symbol);
CryptoSymbol from_string(const std::string &symbol);

#endif // BINANCE_H