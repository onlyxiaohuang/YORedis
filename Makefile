#COMPLIERS
GCC = /usr/bin/gcc
G++ = /usr/bin/g++

#FLAGS
CFLAGS = -O2 -Wall -std=c++17
LDFLAGS = -L./src/libs/libuv/src/unix/ -L./src/libs/libuv/src/win/src/core/ -L./src/libs/libuv/src/win/src -L./src/libs/libuv/src/ -L./src/libs/libuv/src/unix/ -L./src/libs/libuv/src/win/ -L./src/libs/libuv/src/unix/src/ -L./src
LDLIBS = -luv -luv_a


.PHONY: all clean utils server client

#MAKE UTILS
utils:
	$(G++) $(CFLAGS) -c ./utils/err.cpp -o ./output/err.o
	$(G++) $(CFLAGS) -c ./utils/utils.cpp -o ./output/utils.o

#MAKE SERVER
server: server.o
	$(G++) ./output/server.o ./output/err.o ./output/utils.o -o ./output/server
#	$(shell chmod +x ./output/server)
#	$(shell ./output/server)
	
server.o: ./server/main.cpp utils
	$(G++) $(CFLAGS) -c ./server/main.cpp -o ./output/server.o

#MAKE CLIENT
client: client.o
	$(G++) ./output/client.o ./output/err.o ./output/utils.o -o ./output/client
#	$(shell chmod +x ./output/client)
#	$(shell ./output/client)

client.o: ./client/main.cpp utils
	$(G++) $(CFLAGS) -c ./client/main.cpp -o ./output/client.o	

#CLEAN

clean:
	rm -rf ./output/server.o
	rm -rf ./output/err.o
	rm -rf ./output/server
	rm -rf ./output/client.o
	rm -rf ./output/client
	rm -rf ./output/utils.o
