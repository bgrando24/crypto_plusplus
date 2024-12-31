// Header file for declaring the expected structure of Binance's websocket responses
#ifndef BINANCE_H
#define BINANCE_H

#include <string>

/**
 * @brief Enum class to represent different crypto symbols
 */
enum class CryptoSymbol
{
    BTC,
    ETH,
    LTC,
    XRP,
    // Add more symbols as needed
};

/**
 * @brief Struct to represent a Binance AggTrade Websocket response
 */
struct Binance_AggTrade
{
    std::string event;
    long event_time;
    // CryptoSymbol symbol;
    std::string symbol;
    int trade_id;
    double price;
    double quantity;
    long first_trade_id;
    long last_trade_id;
    long trade_time;
    bool is_buyer_maker; // true = buyer placed a limit order (liquidity added), false = seller placed a limit order (liquidity removed)
};

/**
 * @brief Function to convert CryptoSymbol to string
 * @param symbol The CryptoSymbol to convert
 * @return The string representation of the CryptoSymbol
 */
std::string to_string(CryptoSymbol symbol)
{
    switch (symbol)
    {
    case CryptoSymbol::BTC:
        return "BTC";
    case CryptoSymbol::ETH:
        return "ETH";
    case CryptoSymbol::LTC:
        return "LTC";
    case CryptoSymbol::XRP:
        return "XRP";
    // Add more cases as needed
    default:
        return "Unknown";
    }
}

/**
 * @brief Function to convert string to CryptoSymbol
 * @param symbol The string to convert
 * @return The CryptoSymbol representation of the string
 */
CryptoSymbol from_string(const std::string &symbol)
{
    if (symbol == "BTC")
        return CryptoSymbol::BTC;
    if (symbol == "ETH")
        return CryptoSymbol::ETH;
    if (symbol == "LTC")
        return CryptoSymbol::LTC;
    if (symbol == "XRP")
        return CryptoSymbol::XRP;
    // Add more cases as needed
    throw std::invalid_argument("Unknown crypto symbol");
}

#endif