build_server: learning/server_main.cpp learning/server_tcp.h learning/server_udp.h
	g++ -std=c++23 -I. -o bin/server learning/server_main.cpp

build_user: learning/user.cpp
	g++ -std=c++23 -I. -o bin/user learning/user.cpp

test: learning/test.cpp
	g++ -std=c++23 -I. -o bin/test learning/test.cpp && ./bin/test

server: build_server
	./bin/server

user: build_user
	./bin/user
clean:
	rm -f bin/server