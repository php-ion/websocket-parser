#include "websocket_parser.h"
#include <assert.h>
#include <string.h>


#ifdef __GNUC__
# define EXPECTED(X) __builtin_expect(!!(X), 1)
# define UNEXPECTED(X) __builtin_expect(!!(X), 0)
#else
# define EXPECTED(X) (X)
# define UNEXPECTED(X) (X)
#endif

#define SET_STATE(V) parser->state = V
#define HAS_DATA() (p < end )
#define CC (*p)
#define GET_PARSED() ( (p == end) ? len : (p - data) )

#define NOTIFY_CB(FOR)                                                 \
do {                                                                   \
  if (settings->on_##FOR) {                                            \
    if (settings->on_##FOR(parser) != 0) {                             \
      return GET_PARSED();                                             \
    }                                                                  \
  }                                                                    \
} while (0)

#define EMIT_DATA_CB(FOR, ptr, len)                                    \
do {                                                                   \
  if (settings->on_##FOR) {                                            \
    if (settings->on_##FOR(parser, ptr, len) != 0) {                   \
      return GET_PARSED();                                             \
    }                                                                  \
  }                                                                    \
} while (0)

enum state {
    s_start,
    s_head,
    s_length,
    s_mask,
    s_body,
};

void websocket_parser_init(websocket_parser * parser) {
    void *data = parser->data; /* preserve application data */
    memset(parser, 0, sizeof(*parser));
    parser->data = data;
    parser->state = s_start;
    parser->error = 0;
}

void websocket_parser_settings_init(websocket_parser_settings *settings) {
    memset(settings, 0, sizeof(*settings));
}

size_t websocket_parser_execute(websocket_parser *parser, const websocket_parser_settings *settings, const char *data, size_t len) {
    const char * p;
    const char * end = data + len;
    uint8_t header_size = 0;

    for(p = data; p != end; p++) {
        switch(parser->state) {
            case s_start:
                parser->length      = 0;
                parser->mask_offset = 0;
                parser->flags       = (uint32_t) (CC & WS_OP_MASK);
                if(EXPECTED(CC & (1<<7))) {
                    parser->flags |= WS_FIN;
                }
                SET_STATE(s_head);

                header_size++;
                break;
            case s_head:
                parser->length  = (size_t)CC & 0x7F;
                if(CC & 0x80) {
                    parser->flags |= WS_HAS_MASK;
                }
                if(EXPECTED(parser->length >= 126)) {
                    if(EXPECTED(parser->length == 127)) {
                        parser->require = 8;
                    } else {
                        parser->require = 2;
                    }
                    SET_STATE(s_length);
                } else if (EXPECTED(parser->flags & WS_HAS_MASK)) {
                    SET_STATE(s_mask);
                    parser->require = 4;
                } else if (EXPECTED(parser->length)) {
                    SET_STATE(s_body);
                    parser->require = parser->length;
                    NOTIFY_CB(frame_header);
                } else {
                    SET_STATE(s_start);
                    NOTIFY_CB(frame_header);
                    NOTIFY_CB(frame_end);
                }

                header_size++;
                break;
            case s_length:
                while(HAS_DATA() && parser->require) {
                    parser->length <<= 8;
                    parser->length |= (unsigned char)CC;
                    parser->require--;
                    header_size++;
                    p++;
                }
                p--;
                if(UNEXPECTED(!parser->require)) {
                    if (EXPECTED(parser->flags & WS_HAS_MASK)) {
                        SET_STATE(s_mask);
                        parser->require = 4;
                    } else if (EXPECTED(parser->length)) {
                        SET_STATE(s_body);
                        parser->require = parser->length;
                        NOTIFY_CB(frame_header);
                    } else {
                        SET_STATE(s_start);
                        NOTIFY_CB(frame_header);
                        NOTIFY_CB(frame_end);
                    }
                }
                break;
            case s_mask:
                while(HAS_DATA() && parser->require) {
                    parser->mask[4 - parser->require--] = CC;
                    header_size++;
                    p++;
                }
                p--;
                if(UNEXPECTED(!parser->require)) {
                    if(parser->length) {
                        SET_STATE(s_body);
                        parser->require = parser->length;
                        NOTIFY_CB(frame_header);
                    } else {
                        SET_STATE(s_start);
                        NOTIFY_CB(frame_header);
                        NOTIFY_CB(frame_end);
                    }
                }
                break;
            case s_body:
                if(EXPECTED(parser->require)) {
                    if(p + parser->require <= end) {
                        EMIT_DATA_CB(frame_body, p, parser->require);
                        p += parser->require;
                        parser->require = 0;
                    } else {
                        EMIT_DATA_CB(frame_body, p, end - p);
                        parser->require -= end - p;
                        p = end;
                        parser->offset += p - data - header_size;
                    }

                    p--;
                }
                if(UNEXPECTED(!parser->require)) {
                    NOTIFY_CB(frame_end);
                    SET_STATE(s_start);
                }
                break;
            default:
                assert(0 && "Unreachable case");
        }
    }

    return GET_PARSED();
}

void websocket_parser_decode(char * dst, const char * src, size_t len, websocket_parser * parser) {
    size_t i = 0;
    for(; i < len; i++) {
        dst[i] = src[i] ^ parser->mask[(i + parser->mask_offset) % 4];
    }

    parser->mask_offset = (uint8_t) ((i + parser->mask_offset) % 4);
}

uint8_t websocket_decode(char * dst, const char * src, size_t len, char mask[4], uint8_t mask_offset) {
    size_t i = 0;
    for(; i < len; i++) {
        dst[i] = src[i] ^ mask[(i + mask_offset) % 4];
    }

    return (uint8_t) ((i + mask_offset + 1) % 4);
}