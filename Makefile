server: learning/server.cpp
	g++ -std=c++23 -I. -o bin/server learning/server.cpp

user: learning/user.cpp
	g++ -std=c++23 -I. -o bin/user learning/user.cpp

test: learning/test.cpp
	g++ -std=c++23 -I. -o bin/test learning/test.cpp && ./bin/test

run_server: server
	./bin/server

run_server: user
	./bin/user
clean:
	rm -f bin/server