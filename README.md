WebSocket frame parser
======================

### Features
* Fast parsing and building of websocket frame-messages
* No dependencies
* No internal buffering
* No need to buffer the whole frame — works with chunks of a data
* No syscalls
* No allocations
* It can be interrupted at anytime

This is a parser for WebSocket frame-messages (see [RFC6455](https://tools.ietf.org/html/rfc6455)) written in C.

Tested as part of [PHP-ION](https://github.com/php-ion/php-ion) extension.

Inspired by [http-parser](https://github.com/joyent/http-parser) by [Ryan Dahl](https://github.com/ry)
and [multipart-parser](https://github.com/iafonov/multipart-parser-c) by [Igor Afonov](https://github.com/iafonov).

### Usage

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

One websocket_parser object is used per TCP connection. Initialize `websocket_parser` struct using `websocket_parser_init()` and set the callbacks.
That might look something like this for a frame parser:

```c
websocket_parser_settings settings;

websocket_parser_settings_init(&settings);

settings.on_frame_header = websocket_frame_header;
settings.on_frame_body = websocket_frame_body;
settings.on_frame_end = websocket_frame_end;

parser = malloc(sizeof(websocket_parser));
websocket_parser_init(parser);
parser->data = my_frame_struct; // set your custom data after websocket_parser_init() function
```

Basically, callback looks like that:

```c
int websocket_frame_header(websocket_parser * parser) {
    parser->data->opcode = parser->flags & WS_OP_MASK; // gets opcode
    parser->data->is_final = parser->flags & WS_FIN;   // checks is final frame
    if(parser->length) {
        parser->data->body = malloc(parser->length); // allocate memory for frame body, if body exists
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

### Frame builder

Calculate required memory for frame using `websocket_calc_frame_size` function

```c
size_t frame_len = websocket_calc_frame_size(WS_OP_TEXT | WS_FIN | WS_HAS_MASK, data_len);
char * frame = malloc(sizeof(char) * frame_len);
```

build frame

```c
websocket_build_frame(frame, WS_OP_TEXT | WS_FIN | WS_HAS_MASK, mask, data, data_len);
```

and send frame via socket

```c
write(sock, frame, frame_len);
```