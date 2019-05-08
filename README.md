WebSocket frame parser and builder
----------------------------------

This is a parser and builder for WebSocket messages (see [RFC6455](https://tools.ietf.org/html/rfc6455)) written in C.

Table of Contents
-----------------

* [Features](#features)
* [Status](#status)
* [Usage](#usage)
* [Frame builder](#frame-builder)
* [UUID](#uuid)
* [Frame example](#frame-example)

Features
--------

* Fast parsing and building of websocket messages
* No dependencies
* No internal buffering
* No need to buffer the whole frame — works with chunks of a data
* No syscalls
* No allocations
* It can be interrupted at anytime

Tested as part of [PHP-ION](https://github.com/php-ion/php-ion) extension.

Inspired by [http-parser](https://github.com/joyent/http-parser) by [Ryan Dahl](https://github.com/ry)
and [multipart-parser](https://github.com/iafonov/multipart-parser-c) by [Igor Afonov](https://github.com/iafonov).

Status
------

Production ready.

Usage
-----

This parser library works with several callbacks, which the user may set up at application initialization time.

```c
websocket_parser_settings settings;

websocket_parser_settings_init(&settings);

settings.on_frame_header = websocket_frame_header;
settings.on_frame_body = websocket_frame_body;
settings.on_frame_end = websocket_frame_end;
```

These functions must match the signatures defined in the websocket-parser header file.

Returning a value other than 0 from the callbacks will abort message processing.

One websocket_parser object is used per TCP connection. Initialize `websocket_parser` struct using `websocket_parser_init()` and set callbacks:

```c
websocket_parser_settings settings;

websocket_parser_settings_init(&settings);

settings.on_frame_header = websocket_frame_header;
settings.on_frame_body   = websocket_frame_body;
settings.on_frame_end    = websocket_frame_end;

parser = malloc(sizeof(websocket_parser));
websocket_parser_init(parser);
// Attention! Sets your 'data' after websocket_parser_init
parser->data = my_frame_struct; // set your custom data after websocket_parser_init() function
```

Basically, callback looks like that:

```c
int websocket_frame_header(websocket_parser * parser) {
    parser->data->opcode = parser->flags & WS_OP_MASK; // gets opcode
    parser->data->is_final = parser->flags & WS_FIN;   // checks is final frame
    if(parser->length) {
        parser->data->body = malloc(parser->length);   // allocate memory for frame body, if body exists
    }
    return 0;
}

int ion_websocket_frame_body(websocket_parser * parser, const char *at, size_t size) {
    if(parser->flags & WS_HAS_MASK) {
        // if frame has mask, we have to copy and decode data via websocket_parser_copy_masked function
        websocket_parser_decode(&parser->data->body[parser->offset], at, length, parser);
    } else {
        memcpy(&parser->data->body[parser->offset], at, length);
    }
    return 0;
}

int websocket_frame_end(websocket_parser * parser) {
    my_app_push_frame(parser->data); // use parsed frame
}
```

When data is received execute the parser and check for errors.

```c
size_t nread;
// .. init settitngs and parser ... 

nread = websocket_parser_execute(parser, &settings, data, data_len);
if(nread != data_len) {
    // some callback return a value other than 0
}

// ...
free(parser);
```

Frame builder
-------------

To calculate how many bytes to allocate for a frame, use the `websocket_calc_frame_size` function:

```c
size_t frame_len = websocket_calc_frame_size(WS_OP_TEXT | WS_FINAL_FRAME | WS_HAS_MASK, data_len);
char * frame = malloc(sizeof(char) * frame_len);
```

After that you can build a frame

```c
websocket_build_frame(frame, WS_OP_TEXT | WS_FINAL_FRAME | WS_HAS_MASK, mask, data, data_len);
```

and send binary string to the socket

```c
write(sock, frame, frame_len);
```

UUID
----

Macros WEBSOCKET_UUID contains unique ID for handshake

```c
#define WEBSOCKET_UUID   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
```

Frame example
-------------

There is websocket frame example:

* Raw frame: `\x81\x8Amask\x0B\x13\x12\x06\x08\x41\x17\x0A\x19\x00`
* Has mask: yes
* Mask: `mask`
* Payload: `frame data`
* Fin: yes
* Opcode: `WS_OP_TEXT`
