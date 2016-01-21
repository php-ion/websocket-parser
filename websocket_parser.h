#ifndef WEBSOCKET_PARSER_H
#define WEBSOCKET_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif


#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__) && \
  (!defined(_MSC_VER) || _MSC_VER<1600) && !defined(__WINE__)
#include <BaseTsd.h>
#include <stddef.h>
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

typedef struct websocket_parser websocket_parser;
typedef struct websocket_parser_settings websocket_parser_settings;

enum websocket_ops {
    WS_OP_CONTINUE = 0x0,
    WS_OP_TEXT     = 0x1,
    WS_OP_BINARY   = 0x2,
    WS_OP_CLOSE    = 0x8,
    WS_OP_PING     = 0x9,
    WS_OP_PONG     = 0xA,
};

#define WS_OP_MASK 0xF

enum websocket_errors {
    ERR_OK,
    ERR_UNKNOWN_STATE,
};

enum websocket_flags {
    WS_FIN      = 0x10,
    WS_HAS_MASK = 0x20,
};

typedef int (*websocket_data_cb) (websocket_parser*, const char *at, size_t length);
typedef int (*websocket_cb) (websocket_parser*);

struct websocket_parser {
    uint32_t state;
    uint32_t flags;
    uint32_t error;
    char     mask[4];
    uint8_t  mask_offset;

    uint32_t nread;
    size_t length;
    size_t require;
    size_t offset;

    void * data;
};

struct websocket_parser_settings {
    uint64_t          max_frame_size;
    websocket_cb      on_frame_header;
    websocket_data_cb on_frame_body;
    websocket_cb      on_frame_end;
};

void websocket_parser_init(websocket_parser *parser);
void websocket_parser_settings_init(websocket_parser_settings *settings);
size_t websocket_parser_execute(websocket_parser *parser,
                           const websocket_parser_settings *settings,
                           const char *data,
                           size_t len);
void websocket_parser_copy_masked(char * dst, const char * src, size_t len, websocket_parser * parser);
#ifdef __cplusplus
}
#endif
#endif //WEBSOCKET_PARSER_H
