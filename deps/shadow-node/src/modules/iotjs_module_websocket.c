
#include "iotjs_def.h"
#include "iotjs_module_buffer.h"
#include <stdlib.h>
#include <time.h>

#define WS_FINAL_FRAME 0x1
#define WS_NEXT_FRAME 0x2
#define WS_MASKED_FRAME 0x4

#define WS_FIXED_BYTES 2

enum ws_frame_type {
  WS_INCOMPLETE_FRAME = 0x00,
  WS_TEXT_FRAME = 0x01,
  WS_BINARY_FRAME = 0x02,
  WS_OPENING_FRAME = 0x05,
  WS_CLOSING_FRAME = 0x08,
  WS_PING_FRAME = 0x09,
  WS_PONG_FRAME = 0x0A,
  WS_ERROR_FRAME = 0x0f,
};

enum ws_frame_error_type {
  WS_ERROR_OK = 0,
  WS_PARSE_ERROR_INVALID = -1,
  WS_PARSE_ERROR_INCOMPLETE = -2,
};

typedef struct ws_frame {
  uint8_t fin;
  uint8_t rsv1;
  uint8_t rsv2;
  uint8_t rsv3;
  uint8_t opcode;
  uint8_t mask; // always 0 at client
  uint64_t payload_len;
  uint64_t ext_len;    // extended payload length
  int32_t masking_key; // useless at client
  uint8_t* payload;
  enum ws_frame_type type;
} ws_frame;

typedef struct ws_packet {
  uint8_t* data;
  uint64_t len;
} ws_packet;

void w64to8(uint8_t* dstbuffer, uint64_t value, size_t length) {
  if (dstbuffer == NULL) {
    return;
  }

  for (dstbuffer += length - 1; length > 0; length--, dstbuffer--) {
    *dstbuffer = (uint8_t)value;
    value >>= 8;
  }
}

void iotjs_ws_parse_frame_type(struct ws_frame* frame) {
  uint8_t opcode = frame->opcode;
  if (opcode == 0x01) {
    frame->type = WS_TEXT_FRAME;
  } else if (opcode == 0x00) {
    frame->type = WS_INCOMPLETE_FRAME;
  } else if (opcode == 0x02) {
    frame->type = WS_BINARY_FRAME;
  } else if (opcode == 0x08) {
    frame->type = WS_CLOSING_FRAME;
  } else if (opcode == 0x09) {
    frame->type = WS_PING_FRAME;
  } else if (opcode == 0x0A) {
    frame->type = WS_PONG_FRAME;
  } else {
    frame->type = WS_ERROR_FRAME;
  }
}

uint64_t f_uint_parse(uint8_t* value, int offset) {
  uint64_t length = 0;
  for (int i = 0; i < offset; i++) {
    length = (length << 8) | value[i];
  }
  return length;
}

int iotjs_ws_parse_payload_length(ws_packet* packet, struct ws_frame* frame) {
  int length = packet->data[1] & 0x7f;
  if (length < 126) {
    frame->payload_len = (uint64_t)length;
  } else {
    frame->ext_len = length == 126 ? 2 : 8;
    if (packet->len < WS_FIXED_BYTES + frame->ext_len) {
      return WS_PARSE_ERROR_INCOMPLETE;
    }
    frame->payload_len = f_uint_parse(&packet->data[2], frame->ext_len);
  }
  return 0;
}

void iotjs_ws_parse_masking_key(uint8_t* packet, int offset,
                                struct ws_frame* frame) {
  int j = 0;

  uint8_t* masking_key = (uint8_t*)&frame->masking_key;
  int end = offset + 4;
  for (int i = offset; i < end; i++) {
    masking_key[j] = packet[i];
    j++;
  }
}

void iotjs_ws_decode_payload(uint8_t* packet, struct ws_frame* frame) {
  uint8_t* masking_key = (uint8_t*)&frame->masking_key;
  for (uint64_t i = 0; i < frame->payload_len; i++) {
    packet[i] ^= masking_key[i % 4];
  }
  frame->payload = packet;
}

int iotjs_ws_parse_payload(ws_packet* packet, struct ws_frame* frame) {
  int r = iotjs_ws_parse_payload_length(packet, frame);
  if (r != WS_ERROR_OK) {
    return r;
  }
  if (frame->mask == 1) {
    // messages from server should not mask data, so we don't need unmask data
    return WS_PARSE_ERROR_INVALID;
  } else {
    if (packet->len < WS_FIXED_BYTES + frame->ext_len + frame->payload_len) {
      return WS_PARSE_ERROR_INCOMPLETE;
    }
    frame->payload = &packet->data[WS_FIXED_BYTES + frame->ext_len];
  }
  return 0;
}

int iotjs_ws_parse_input(ws_packet* packet, struct ws_frame* frame) {
  if (packet->data == NULL)
    return WS_PARSE_ERROR_INVALID;
  if (packet->len < WS_FIXED_BYTES)
    return WS_PARSE_ERROR_INVALID;
  memset(frame, 0, sizeof(struct ws_frame));
  frame->fin = packet->data[0] >> 7;
  frame->rsv1 = packet->data[0] >> 6 & 0x01;
  frame->rsv2 = packet->data[0] >> 5 & 0x01;
  frame->rsv3 = packet->data[0] >> 4 & 0x01;
  frame->opcode = packet->data[0] & 0x0f;
  frame->mask = packet->data[1] >> 7;
  iotjs_ws_parse_frame_type(frame);
  if (frame->type != WS_ERROR_FRAME) {
    return iotjs_ws_parse_payload(packet, frame);
  } else {
    return WS_PARSE_ERROR_INVALID;
  }
}

uint8_t* iotjs_ws_make_header(size_t data_len, enum ws_frame_type frame_type,
                              size_t* header_len, int options) {
  uint8_t* header;
  uint8_t end_byte;
  uint8_t masked = 0x80;

  if (data_len < 1) {
    return NULL;
  }

  if ((options & WS_FINAL_FRAME) == 0x0) {
    end_byte = 0x0;
  } else if (options & WS_FINAL_FRAME) {
    end_byte = 0x80;
  } else {
    return NULL;
  }
  *header_len = 0;

  if (data_len < 126) {
    header = malloc(sizeof(uint8_t) * 2);
    header[0] = end_byte | frame_type;
    header[1] = (uint8_t)(masked | data_len);
    *header_len = 2;
  } else if (data_len > 126 && data_len < 65536) {
    header = malloc(sizeof(uint8_t) * 4);
    header[0] = end_byte | frame_type;
    header[1] = (uint8_t)(masked | 0x7e);
    header[2] = (uint8_t)(data_len >> 8);
    header[3] = (uint8_t)data_len & 0xff;
    *header_len = 4;
  } else if (data_len >= 65536) {
    header = malloc(sizeof(uint8_t) * 10);
    header[0] = end_byte | frame_type;
    header[1] = (uint8_t)(masked | 0x7f);
    w64to8(&header[2], data_len, 8);
    *header_len = 10;
  } else {
    return NULL;
  }

  int offset = (int)*header_len;
  header[offset + 0] = rand() / (RAND_MAX / 0xff);
  header[offset + 1] = rand() / (RAND_MAX / 0xff);
  header[offset + 2] = rand() / (RAND_MAX / 0xff);
  header[offset + 3] = rand() / (RAND_MAX / 0xff);

  *header_len += 4;
  return header;
}


JS_FUNCTION(EncodeFrame) {
  int type = JS_GET_ARG(0, number);
  iotjs_bufferwrap_t* data = iotjs_bufferwrap_from_jbuffer(jargv[1]);

  uint8_t* header;
  size_t header_len;
  size_t data_len = iotjs_bufferwrap_length(data);
  header = iotjs_ws_make_header(data_len, (enum ws_frame_type)type, &header_len,
                                WS_FINAL_FRAME);

  size_t out_len = data_len + header_len;
  uint8_t out_frame[out_len + 1];

  memset(out_frame, 0, out_len + 1);
  memcpy(out_frame, header, header_len);

  uint8_t* mask = header + header_len - 4;
  char* masked = iotjs_bufferwrap_buffer(data);

  for (size_t i = 0; i < data_len; ++i) {
    out_frame[header_len + i] = (uint8_t)(masked[i] ^ mask[i % 4]);
  }
  jerry_value_t jframe = iotjs_bufferwrap_create_buffer(data_len + header_len);
  iotjs_bufferwrap_t* frame_wrap = iotjs_bufferwrap_from_jbuffer(jframe);
  iotjs_bufferwrap_copy(frame_wrap, (const char*)out_frame, out_len);

  free(header);
  return jframe;
}

JS_FUNCTION(DecodeFrame) {
  iotjs_bufferwrap_t* data = iotjs_bufferwrap_from_jbuffer(jargv[0]);
  uint8_t* chunk = (uint8_t*)iotjs_bufferwrap_buffer(data);
  size_t chunk_size = iotjs_bufferwrap_length(data);

  jerry_value_t ret = jerry_create_object();
  struct ws_packet packet;
  packet.data = chunk;
  packet.len = chunk_size;
  struct ws_frame frame;
  int r = iotjs_ws_parse_input(&packet, &frame);
  if (r != WS_ERROR_OK) {
    iotjs_jval_set_property_jval(ret, "err_code", jerry_create_number(r));
    return ret;
  }

  int total_len = frame.payload_len + frame.ext_len + WS_FIXED_BYTES;

  jerry_value_t jbuffer = iotjs_bufferwrap_create_buffer(frame.payload_len);
  iotjs_bufferwrap_t* buffer_wrap = iotjs_bufferwrap_from_jbuffer(jbuffer);
  iotjs_bufferwrap_copy(buffer_wrap, (const char*)frame.payload,
                        frame.payload_len);

  iotjs_jval_set_property_jval(ret, "type",
                               jerry_create_number((int)frame.type));
  iotjs_jval_set_property_jval(ret, "fin", jerry_create_number(frame.fin));
  iotjs_jval_set_property_jval(ret, "opcode",
                               jerry_create_number(frame.opcode));
  iotjs_jval_set_property_jval(ret, "mask", jerry_create_number(frame.mask));
  iotjs_jval_set_property_jval(ret, "masking_key",
                               jerry_create_number(frame.masking_key));
  iotjs_jval_set_property_jval(ret, "total_len",
                               jerry_create_number(total_len));
  iotjs_jval_set_property_jval(ret, "buffer", jbuffer);
  iotjs_jval_set_property_jval(ret, "err_code",
                               jerry_create_number((int)WS_ERROR_OK));
  jerry_release_value(jbuffer);

  return ret;
}

jerry_value_t InitWebSocket() {
  srand((unsigned int)time(NULL));

  jerry_value_t exports = jerry_create_object();
  iotjs_jval_set_method(exports, "encodeFrame", EncodeFrame);
  iotjs_jval_set_method(exports, "decodeFrame", DecodeFrame);
  return exports;
}
