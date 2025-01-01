#include "../include/binance.h"

/**
 * @brief Convert a CryptoSymbol to a string
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
 * @brief Convert a string to a CryptoSymbol
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