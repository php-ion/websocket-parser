WebSocket frame parser
======================


### Features
* No dependencies
* Works with chunks of a data - no need to buffer the whole request

This is a parser for WebSocket frame-messages written in C (by [RFC6455](https://tools.ietf.org/html/rfc6455)).
It does not make any syscalls nor allocations, it does not
buffer data, it can be interrupted at anytime.

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

Initialize the `struct` using `http_parser_init()` and set the callbacks. That might look something
like this for a request parser:
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
        websocket_parser_copy_masked(&parser->data->body[parser->offset], at, size, parser);
    } else {
        memcpy(&parser->data->body[parser->offset], at, size);
    }
    return 0;
}

int websocket_frame_end(websocket_parser * parser) {
    my_app_push_frame(parser->data); // use parsed frame
}
```

When data is received execute the parser and check for errors.

```c
websocket_parser_execute(parser, &settings, data, data_len);
if(parser->error) {
    // e.g. log error
}
// ...
free(parser);
```
