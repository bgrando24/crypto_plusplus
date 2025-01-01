// filepath: /Users/brandongrando/Documents/High_Performance_C++/crypto_plusplus/include/binance.h
#ifndef BINANCE_H
#define BINANCE_H

#include <string>
#include <stdexcept>

enum class CryptoSymbol
{
    BTC,
    ETH,
    LTC,
    XRP,
    // Add more symbols as needed
};

struct Binance_AggTrade
{
    std::string event;
    long event_time;
    std::string symbol;
    int trade_id;
    double price;
    double quantity;
    long first_trade_id;
    long last_trade_id;
    long trade_time;
    bool is_buyer_maker;
};

std::string to_string(CryptoSymbol symbol);
CryptoSymbol from_string(const std::string &symbol);

#endif // BINANCE_H