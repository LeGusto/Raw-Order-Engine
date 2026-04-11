#include "order_book.h"
#include <optional>
#include <iostream>

std::optional<Order> OrderBook::highest_bid()
{
    if (this->bidMap.empty())
        return std::nullopt;

    return *bidMap.begin()->second.begin();
}

std::optional<Order> OrderBook::lowest_ask()
{
    if (this->askMap.empty())
        return std::nullopt;

    return *askMap.begin()->second.begin();
}

std::vector<Match> OrderBook::match_orders()
{
    std::vector<Match> res;
    while (!askMap.empty() && !bidMap.empty())
    {
        auto ptr_a = &askMap.begin()->second;
        auto ptr_b = &bidMap.begin()->second;
        if (ptr_a->begin()->price <= ptr_b->begin()->price)
        {
            uint32_t qty = std::min(ptr_a->begin()->quantity, ptr_b->begin()->quantity);

            res.push_back({*ptr_a->begin(), *ptr_b->begin(), qty});

            if (ptr_a->begin()->quantity == qty)
            {
                this->orderIDMap.erase(ptr_a->begin()->id);
                ptr_a->pop_front();
                if (ptr_a->empty())
                    askMap.erase(askMap.begin());
            }
            else
            {
                ptr_a->begin()->quantity -= qty;
            }

            if (ptr_b->begin()->quantity == qty)
            {
                this->orderIDMap.erase(ptr_b->begin()->id);
                ptr_b->pop_front();
                if (ptr_b->empty())
                    bidMap.erase(bidMap.begin());
            }
            else
                ptr_b->begin()->quantity -= qty;
        }
        else
            break;
    }

    return res;
}

std::variant<std::vector<Match>, Order> OrderBook::process_order(SIDE side, uint32_t quantity, uint32_t price, uint32_t customerID)
{
    Order order{side, quantity, price, customerID};
    std::list<Order>::iterator order_it;
    if (side == SIDE::ASK)
    {
        if (askMap.find(order.price) == askMap.end())
        {
            askMap.insert({});
        }
        askMap[order.price].push_back(order);
        order_it = askMap[order.price].end();
    }
    else if (side == SIDE::BID)
    {
        if (bidMap.find(order.price) == bidMap.end())
        {
            bidMap.insert({});
        }
        bidMap[order.price].push_back(order);
        order_it = bidMap[order.price].end();
    }

    advance(order_it, -1);
    customerIDMap[customerID].push_back(order_it);

    std::list<std::list<Order>::iterator>::iterator customer_it = customerIDMap[customerID].end();
    advance(customer_it, -1);

    mapNavigation entry = {order_it, customer_it, order};
    orderIDMap.insert({order.id, entry});
    ;

    std::vector<Match> matches = match_orders();
    if (!matches.empty())
        return matches;

    return order;
}

std::optional<Order> OrderBook::cancel_order(uint32_t orderID)
{
    if (orderIDMap.find(orderID) == orderIDMap.end())
        return std::nullopt;

    auto found = orderIDMap.find(orderID);
    mapNavigation item = found->second;

    Order rt = *item.order_it;

    if (item.side == SIDE::BID)
    {
        bidMap[item.price].erase(item.order_it);
        if (bidMap[item.price].empty())
            bidMap.erase(item.price);
    }
    else if (item.side == SIDE::ASK)
    {
        askMap[item.price].erase(item.order_it);
        if (askMap[item.price].empty())
            askMap.erase(item.price);
    }

    orderIDMap.erase(orderID);
    customerIDMap[rt.customerID].erase(item.customer_it);
    if (customerIDMap[rt.customerID].empty())
        customerIDMap.erase(rt.customerID);

    return rt;
}

std::vector<Order> OrderBook::get_orders(uint32_t customerID)
{
    std::vector<Order> rt;
    if (customerIDMap.find(customerID) == customerIDMap.end())
        return rt;

    auto it = customerIDMap[customerID].begin();
    while (it != customerIDMap[customerID].end())
    {
        rt.push_back(*(*it));
        advance(it, 1);
    }
    return rt;
}