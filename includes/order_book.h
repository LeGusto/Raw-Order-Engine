#pragma once

#include <cstdint>
#include <map>
#include <list>
#include <vector>
#include <optional>
#include <variant>

enum class MessageType : uint8_t
{
    SUBMIT_ORDER = 0x01,
    CANCEL_ORDER = 0x02,
    ORDER_ACK = 0x03,
    CANCEL_ACK = 0x04,
    MATCH = 0x05,
    REJECT = 0x06,
};

enum class SIDE
{
    ASK,
    BID
};

struct Order
{
private:
    inline static uint32_t orderID = 0;

public:
    uint32_t id;
    uint32_t quantity = 0;
    uint32_t price;
    uint32_t customerID;
    SIDE side;

    Order(SIDE _side, uint32_t _quantity, uint32_t _price, uint32_t _customerID) : side(_side), quantity(_quantity), price(_price), customerID(_customerID), id(orderID++) {};

    auto fields() const
    {
        return std::tie(id, quantity, price, customerID, side); // generate tuple<uint32_t&,...>
    }

    auto fields()
    {
        return std::tie(id, quantity, price, customerID, side); // generate tuple<uint32_t&,...>
    }
};

struct Match
{
public:
    Order askID;
    Order bidID;
    uint32_t quantity = 0;

    auto fields() const
    {
        return std::tie(askID, bidID, quantity); // generate tuple<uint32_t&,...>
    }

    Match(Order _askID, Order _bidID, uint32_t _quantity) : askID(_askID), bidID(_bidID), quantity(_quantity) {};
};

struct mapNavigation
{
    std::list<Order>::iterator order_it;
    std::list<std::list<Order>::iterator>::iterator customer_it;
    SIDE side;
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

public:
    std::optional<Order> highest_bid();
    std::optional<Order> lowest_ask();

    std::variant<std::vector<Match>, Order> process_order(SIDE side, uint32_t quantity, uint32_t price, uint32_t customerID);
    std::optional<Order> cancel_order(uint32_t orderID);

    std::vector<Order> get_orders(uint32_t customerID);
};