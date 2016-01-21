CFLAGS?=-std=gnu99 -ansi -pedantic -O4 -Wall -fPIC

default: websocket_parser.o

websocket_parser.o: websocket_parser.c websocket_parser.h

solib: websocket_parser.o
	$(CC) -shared -Wl,-soname,libwebsocket_parser.so -o libwebsocket_parser.so websocket_parser.o

clean:
	rm -f *.o *.so
