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

void OrderBook::remove_order_refs(uint32_t orderID)
{
    auto &nav = orderIDMap.at(orderID);
    customerIDMap[nav.order_it->customerID].erase(nav.customer_it);
    if (customerIDMap[nav.order_it->customerID].empty())
        customerIDMap.erase(nav.order_it->customerID);
    orderIDMap.erase(orderID);
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
                remove_order_refs(ptr_a->begin()->id);
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
                remove_order_refs(ptr_b->begin()->id);
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

std::variant<std::vector<Match>, Order> OrderBook::process_order(Side side, uint32_t quantity, uint32_t price, uint32_t customerID)
{
    Order order{side, quantity, price, customerID};
    std::list<Order>::iterator order_it;
    if (side == Side::ASK)
    {
        askMap[order.price].push_back(order);
        order_it = askMap[order.price].end();
    }
    else if (side == Side::BID)
    {
        bidMap[order.price].push_back(order);
        order_it = bidMap[order.price].end();
    }

    advance(order_it, -1);
    customerIDMap[customerID].push_back(order_it);

    std::list<std::list<Order>::iterator>::iterator customer_it = customerIDMap[customerID].end();
    advance(customer_it, -1);

    mapNavigation entry = {order_it, customer_it, order};
    orderIDMap.insert({order.id, entry});

    std::vector<Match> matches = match_orders();
    if (!matches.empty())
        return matches;

    return order;
}

std::optional<Order> OrderBook::cancel_order(uint32_t orderID)
{
    auto found = orderIDMap.find(orderID);
    if (found == orderIDMap.end())
        return std::nullopt;

    mapNavigation item = found->second;

    Order rt = *item.order_it;

    remove_order_refs(orderID);

    if (item.side == Side::BID)
    {
        bidMap[item.price].erase(item.order_it);
        if (bidMap[item.price].empty())
            bidMap.erase(item.price);
    }
    else if (item.side == Side::ASK)
    {
        askMap[item.price].erase(item.order_it);
        if (askMap[item.price].empty())
            askMap.erase(item.price);
    }

    return rt;
}

std::vector<Order> OrderBook::get_orders(uint32_t customerID)
{
    std::vector<Order> rt;
    if (customerIDMap.find(customerID) == customerIDMap.end())
        return rt;

    rt.reserve(customerIDMap[customerID].size());

    auto it = customerIDMap[customerID].begin();
    while (it != customerIDMap[customerID].end())
    {
        rt.push_back(*(*it));
        advance(it, 1);
    }
    return rt;
}