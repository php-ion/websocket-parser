CFLAGS?=-std=gnu99 -pedantic -O4 -Wall -fPIC

default: websocket_parser.o

websocket_parser.o: websocket_parser.c websocket_parser.h

solib: websocket_parser.o
	$(CC) -shared -Wl,-soname,libwebsocket_parser.so -o libwebsocket_parser.so websocket_parser.o

alib: websocket_parser.o
	ar rcu libwebsocket_parser.a $<
	ranlib libwebsocket_parser.a

clean:
	rm -f *.o *.so *.a
