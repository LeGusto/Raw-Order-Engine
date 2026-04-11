#pragma once

#include <cstdint>

enum class MessageType : uint8_t
{
    SUBMIT_ORDER = 0x01,
    CANCEL_ORDER = 0x02,
    ORDER_ACK = 0x03,
    CANCEL_ACK = 0x04,
    MATCH = 0x05,
    REJECT = 0x06,
    GET_ORDERS = 0x07,
    ORDERS_LIST = 0x08,
};

constexpr uint32_t MAX_REQUEST_SIZE = 128000;
