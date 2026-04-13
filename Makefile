CXXFLAGS = -std=c++23 -I. -Isrc -Iincludes -Wall -Wextra -Werror -g -fsanitize=address,undefined

build_server: src/server_base.cpp src/server_main.cpp src/server_tcp.cpp src/server_udp.cpp src/serializer.cpp src/order_book.cpp
	g++ $(CXXFLAGS) -o bin/server src/server_main.cpp src/server_base.cpp src/server_tcp.cpp src/server_udp.cpp src/serializer.cpp src/order_book.cpp


build_user: src/user.cpp src/user_spawner.cpp src/serializer.cpp
	g++ ${CXXFLAGS} -o bin/user src/user_spawner.cpp src/user.cpp src/serializer.cpp

test: learning/test.cpp
	g++ ${CXXFLAGS} -o bin/test learning/test.cpp && ./bin/test

server: build_server
	./bin/server

user: build_user
	./bin/user

build_order_book_test: tests/test_order_book.cpp src/order_book.cpp
	g++ ${CXXFLAGS} -o bin/test_order_book tests/test_order_book.cpp src/order_book.cpp

test_order_book: build_order_book_test
	./bin/test_order_book

build_serializer_test: tests/test_serializer.cpp includes/serializer.h src/serializer.cpp
	g++ ${CXXFLAGS} -o bin/test_serializer tests/test_serializer.cpp src/serializer.cpp


test_serializer: build_serializer_test
	./bin/test_serializer


clean:
	rm -f bin/*