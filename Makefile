#COMPLIERS
GCC = /usr/bin/gcc
G++ = /usr/bin/g++

#FLAGS
CFLAGS = -O2 -Wall -std=c++17
LDFLAGS = -L./src/libs/libuv/src/unix/ -L./src/libs/libuv/src/win/src/core/ -L./src/libs/libuv/src/win/src -L./src/libs/libuv/src/ -L./src/libs/libuv/src/unix/ -L./src/libs/libuv/src/win/ -L./src/libs/libuv/src/unix/src/ -L./src
LDLIBS = -luv -luv_a

#MAKE SERVER
PHONY:server.o server clean

server: server.o
	$(G++) ./output/server.o ./output/err.o -o ./output/server
	$(shell chmod +x ./output/server)
	$(shell ./output/server)
	
server.o: ./server/main.cpp ./server/err.hpp
	$(G++) $(FLAGS) -c ./server/err.cpp -o ./output/err.o
	$(G++) $(FLAGS) -c ./server/main.cpp -o ./output/server.o

clean:
	rm -rf ./output/server.o
	rm -rf ./output/err.o