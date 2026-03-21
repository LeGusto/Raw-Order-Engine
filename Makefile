build_server: learning/server.cpp
	g++ -std=c++23 -I. -o bin/server learning/server.cpp

build_user: learning/user.cpp
	g++ -std=c++23 -I. -o bin/user learning/user.cpp

test: learning/test.cpp
	g++ -std=c++23 -I. -o bin/test learning/test.cpp && ./bin/test

run_server: build_server
	./bin/server

run_user: build_user
	./bin/user
clean:
	rm -f bin/server