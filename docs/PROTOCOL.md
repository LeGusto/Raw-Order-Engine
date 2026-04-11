# Order Book Wire Protocol

All multi-byte integers are network byte order.

## Message Framing

```
 0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Message Length        |     Type      |   Payload...  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field          | Size    | Description                    |
|----------------|---------|--------------------------------|
| Message Length | 2 bytes | Total payload size (excl. self)|
| Type           | 1 byte  | Message type ID                |
| Payload        | varies  | Type-specific fields           |

## Message Types

| Type ID | Name             | Direction        |
|---------|------------------|------------------|
| 0x01    | SUBMIT_ORDER     | Client -> Server |
| 0x02    | CANCEL_ORDER     | Client -> Server |
| 0x03    | ORDER_ACK        | Server -> Client |
| 0x04    | CANCEL_ACK       | Server -> Client |
| 0x05    | MATCH            | Server -> Client |
| 0x06    | REJECT           | Server -> Client |

## 0x01 SUBMIT_ORDER

```
 0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Customer ID                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Side      |                    Price                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |                   Quantity                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |
+-+-+-+-+-+-+-+-+
```

| Field       | Offset | Size    | Description            |
|-------------|--------|---------|------------------------|
| Customer ID | 0      | 4 bytes | uint32, client ID      |
| Side        | 4      | 1 byte  | 0x00 = ASK, 0x01 = BID|
| Price       | 5      | 4 bytes | uint32, price level    |
| Quantity    | 9      | 4 bytes | uint32, order quantity  |

Total: 13 bytes

## 0x02 CANCEL_ORDER

| Field    | Offset | Size    | Description       |
|----------|--------|---------|-------------------|
| Order ID | 0      | 4 bytes | uint32, order ID  |

Total: 4 bytes

## 0x03 ORDER_ACK

| Field    | Offset | Size    | Description                |
|----------|--------|---------|----------------------------|
| Order ID | 0      | 4 bytes | uint32, assigned order ID  |

Total: 4 bytes

## 0x04 CANCEL_ACK

| Field    | Offset | Size    | Description              |
|----------|--------|---------|--------------------------|
| Order ID | 0      | 4 bytes | uint32, cancelled order  |

Total: 4 bytes

## 0x05 MATCH

Sent to both the buyer and seller when orders match.

| Field        | Offset | Size    | Description              |
|--------------|--------|---------|--------------------------|
| Ask Order ID | 0      | 4 bytes | uint32, ask side order   |
| Bid Order ID | 4      | 4 bytes | uint32, bid side order   |
| Price        | 8      | 4 bytes | uint32, matched price    |
| Quantity     | 12     | 4 bytes | uint32, matched quantity |

Total: 16 bytes

## 0x06 REJECT

| Field       | Offset | Size    | Description                |
|-------------|--------|---------|----------------------------|
| Order ID    | 0      | 4 bytes | uint32, rejected order     |
| Reason Code | 4      | 1 byte  | 0x01 = invalid side        |
|             |        |         | 0x02 = invalid quantity    |
|             |        |         | 0x03 = unknown order (cancel) |

Total: 5 bytes
