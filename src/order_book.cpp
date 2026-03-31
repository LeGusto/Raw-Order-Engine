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
            int qty = std::min(ptr_a->begin()->quantity, ptr_b->begin()->quantity);

            res.push_back({*ptr_a->begin(), *ptr_a->begin(), qty});

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

std::vector<Match> OrderBook::process_order(SIDE side, uint32_t quantity, uint32_t price, uint32_t customerID)
{
    Order order{side, quantity, price, customerID};
    if (side == SIDE::ASK)
    {
        if (askMap.find(order.price) == askMap.end())
        {
            askMap.insert({});
        }
        askMap[order.price].push_back(order);
    }
    else if (side == SIDE::BID)
    {
        if (bidMap.find(order.price) == bidMap.end())
        {
            bidMap.insert({});
        }
        bidMap[order.price].push_back(order);
    }

    return match_orders();
}