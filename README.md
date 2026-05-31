# Order Book Matching Engine

An order book server built from raw TCP sockets in C++23, written to learn systems programming and networking concepts.

## What It Does

Clients connect over TCP and submit buy or sell orders. The server matches them when the prices cross — a sell price at or below a buy price — and notifies both sides. Unmatched orders sit in the book until they're filled or cancelled.

## Architecture

```
src/
├── server_main.cpp       Entry point
├── server_base.cpp       Socket setup, signal handling
├── server_tcp.cpp        poll()-based event loop, up to 20 clients
├── order_book.cpp        Matching engine
├── serializer.cpp        Binary encode/decode
├── latency.cpp           Per-operation latency tracking
├── user.cpp              Stress test client
└── user_spawner.cpp      Spawns client threads

includes/
├── order_book.h          Order, Match, OrderBook types
├── protocol.h            Wire message types and payloads
├── serializer.h          Template serialization
├── server_tcp.h / udp.h  Server interfaces
└── tcp_helpers.h         send/recv wrappers

tests/
├── test_order_book.cpp   Matching logic unit tests
├── test_serializer.cpp   Pack/unpack round-trip tests
└── test_stress.cpp       Randomized stress test with invariant checks
```

### Order Book

The book is split into two sides:

| Side | Container | Order |
|------|-----------|-------|
| ASK (sell) | `std::map<price, list<Order>>` | ascending — cheapest ask first |
| BID (buy) | `std::map<price, list<Order>, std::greater<>>` | descending — highest bid first |

Fast cancellation is achieved via `orderIDMap`, which stores iterators directly into both maps, giving O(log n) lookup and O(1) removal.

Matching fires whenever `best_ask_price <= best_bid_price`. Partial fills are supported — the remainder stays resting in the book.

### Network Layer

The server uses a single-threaded `poll()` loop over all file descriptors — no blocking calls, no per-client threads. Each client gets a `ClientBuffer` that accumulates bytes until a full framed message arrives, then dispatches it to the order book.

### Serialization

A compile-time generic serializer (`includes/serializer.h`) handles all wire encoding:

- Integers -> network byte order
- Enums -> underlying integer type
- Floats / Doubles -> IEEE 754 bit encoding
- Containers -> length-prefixed sequences
- Structures -> recursive serialization of underlying members

## Wire Protocol

Binary framing over TCP. All multi-byte integers are network byte order. 

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Message Length                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |                 Payload...                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Type | Name | Direction | Payload |
|------|------|-----------|---------|
| `0x01` | `SUBMIT_ORDER` | C → S | customer\_id(4) + side(1) + price(4) + qty(4) |
| `0x02` | `CANCEL_ORDER` | C → S | order\_id(4) |
| `0x03` | `ORDER_ACK` | S → C | order\_id(4) |
| `0x04` | `CANCEL_ACK` | S → C | order\_id(4) |
| `0x05` | `MATCH` | S → C | ask\_order(17) + bid\_order(17) + qty(4) |
| `0x06` | `REJECT` | S → C | reason\_code(1) |
| `0x07` | `GET_ORDERS` | C → S | customer\_id(4) |
| `0x08` | `ORDERS_LIST` | S → C | count(4) + orders(17 × n) |

Full protocol spec: [docs/PROTOCOL.md](docs/PROTOCOL.md)

## Building

**Dependencies:** CMake >= 3.20, a C++23 compiler, Intel TBB, pthreads.

```bash
# Debug build 
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

# Thread sanitizer build
cmake -B build-tsan -DCMAKE_BUILD_TYPE=TSan
cmake --build build-tsan
```

Binaries land in `bin/`.

## Running

```bash
# Terminal 1 — start the server 
./bin/server

# Terminal 2 — run stress test 
./bin/user
```

## Testing

```bash
cd build && ctest --output-on-failure
```

| Test | What it covers |
|------|---------------|
| `test_order_book` | Exact matches, partial fills, no-match spreads, cancellations |
| `test_serializer` | Pack/unpack round-trips for all supported types |
| `test_stress` | Randomized operations with invariant verification |

## Benchmarking

Run the server and stress tester multiple times under a label, with warmup runs discarded:

```bash
# N runs (default 10) + 2 warmup runs, results saved to bench_results/<label>/
scripts/bench.sh <label>
scripts/bench.sh 20 <label>
```

Analyze the results:

```bash
# Stats for a single label
scripts/analyze.py bench_results/<label>

# Compare two labels
scripts/analyze.py --compare bench_results/<label_a> bench_results/<label_b>
```

Results are saved to `bench_results/` and latency logs to `logs/`.

## Configuration

Edit [config.h](config.h) to tune:

| Constant | Default | Description |
|----------|---------|-------------|
| `PORT` | `"8089"` | Listening port |
| `BACKLOG` | `20` | Max pending connections |
| `NUM_USERS` | `10` | Concurrent stress-test threads |
| `USER_REQUESTS` | `100000` | Requests per client thread |
| `LOG_ENTRIES` | `2000000` | Latency tracker ring buffer size |
| `DEBUG` | `false` | Verbose logging |
