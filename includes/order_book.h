#pragma once

#include <cstdint>
#include <map>
#include <list>
#include <vector>
#include <optional>
#include <variant>
#include <format>
#include <string>
#include <iostream>
#include <atomic>

enum class Side : uint8_t
{
    ASK,
    BID
};

struct Order
{
private:
    inline static std::atomic<uint32_t> orderID = 0;

public:
    uint32_t id;
    uint32_t quantity = 0;
    uint32_t price;
    uint32_t customerID;
    Side side;

    Order(Side _side, uint32_t _quantity, uint32_t _price, uint32_t _customerID) : id(orderID++), quantity(_quantity), price(_price), customerID(_customerID), side(_side) {};

    Order() = default;

    auto fields() const
    {
        return std::tie(id, quantity, price, customerID, side); // generate tuple<uint32_t&,...>
    }

    auto fields()
    {
        return std::tie(id, quantity, price, customerID, side); // generate tuple<uint32_t&,...>
    }

    void print() const
    {
        std::cout << "Order(id=" << id
                  << ", side=" << (side == Side::ASK ? "ASK" : "BID")
                  << ", qty=" << quantity
                  << ", price=" << price
                  << ", customer=" << customerID << ")\n";
    }
};

struct Match
{
public:
    Order ask_order;
    Order bid_order;
    uint32_t quantity = 0;

    auto fields() const
    {
        return std::tie(ask_order, bid_order, quantity);
    }

    auto fields()
    {
        return std::tie(ask_order, bid_order, quantity);
    }

    Match() = default;
    Match(Order _ask_order, Order _bid_order, uint32_t _quantity) : ask_order(_ask_order), bid_order(_bid_order), quantity(_quantity) {};
};

struct mapNavigation
{
    std::list<Order>::iterator order_it;
    std::list<std::list<Order>::iterator>::iterator customer_it;
    Side side;
    uint32_t price;

    mapNavigation(std::list<Order>::iterator _order_it, std::list<std::list<Order>::iterator>::iterator _customer_it, Order &order) : order_it(_order_it), customer_it(_customer_it), side(order.side), price(order.price) {};
};

class OrderBook
{
private:
    std::map<uint32_t, std::list<Order>> askMap;
    std::map<uint32_t, std::list<Order>, std::greater<uint32_t>> bidMap;

    std::map<uint32_t, mapNavigation> orderIDMap;

    std::map<uint32_t, std::list<std::list<Order>::iterator>> customerIDMap;

    std::vector<Match> match_orders();
    void remove_order_refs(uint32_t orderID);

public:
    std::optional<Order> highest_bid();
    std::optional<Order> lowest_ask();

    std::variant<std::vector<Match>, Order> process_order(Side side, uint32_t quantity, uint32_t price, uint32_t customerID);
    std::optional<Order> cancel_order(uint32_t orderID);

    std::vector<Order> get_orders(uint32_t customerID);

    std::string check_invariants() const;
};