#ifndef IOTJS_BASE64_H_
#define IOTJS_BASE64_H_

#include <stddef.h>
#include <stdint.h>

#define base64_encoded_size(size) ((size + 2 - ((size + 2) % 3)) / 3 * 4)

// Doesn't check for padding at the end.  Can be 1-2 bytes over.
static inline size_t base64_decoded_size_fast(size_t size) {
  size_t remainder = size % 4;

  size = (size / 4) * 3;
  if (remainder) {
    if (size == 0 && remainder == 1) {
      // special case: 1-byte input cannot be decoded
      size = 0;
    } else {
      // non-padded input, add 1 or 2 extra bytes
      size += 1 + (size_t)(remainder == 3);
    }
  }

  return size;
}

size_t base64_decoded_size(const char* src, size_t size) {
  if (size == 0)
    return 0;

  if (src[size - 1] == '=')
    size--;
  if (size > 0 && src[size - 1] == '=')
    size--;

  return base64_decoded_size_fast(size);
}

extern const int8_t unbase64_table[256];

#define unbase64(x) (uint8_t)(unbase64_table[(uint8_t)x])


bool base64_decode_group_slow(char* const dst, const size_t dstlen,
                              const char* const src, const size_t srclen,
                              size_t* i, size_t* k) {
  uint8_t hi;
  uint8_t lo;
#define V(expr)                               \
  for (;;) {                                  \
    const uint8_t c = (const uint8_t)src[*i]; \
    lo = unbase64(c);                         \
    *i += 1;                                  \
    if (lo < 64)                              \
      break; /* Legal character. */           \
    if (c == '=' || *i >= srclen)             \
      return false; /* Stop decoding. */      \
  }                                           \
  expr;                                       \
  if (*i >= srclen)                           \
    return false;                             \
  if (*k >= dstlen)                           \
    return false;                             \
  hi = lo;
  V(/* Nothing. */);
  V(dst[(*k)++] = ((hi & 0x3F) << 2) | ((lo & 0x30) >> 4));
  V(dst[(*k)++] = ((hi & 0x0F) << 4) | ((lo & 0x3C) >> 2));
  V(dst[(*k)++] = ((hi & 0x03) << 6) | ((lo & 0x3F) >> 0));
#undef V
  return true; // Continue decoding.
}


size_t base64_decode_fast(char* const dst, const size_t dstlen,
                          const char* const src, const size_t srclen,
                          const size_t decoded_size) {
  const size_t available = dstlen < decoded_size ? dstlen : decoded_size;
  const size_t max_k = available / 3 * 3;
  size_t max_i = srclen / 4 * 4;
  size_t i = 0;
  size_t k = 0;
  while (i < max_i && k < max_k) {
    const uint32_t v =
        (uint32_t)(unbase64(src[i + 0]) << 24 | unbase64(src[i + 1]) << 16 |
                   unbase64(src[i + 2]) << 8 | unbase64(src[i + 3]));
    // If MSB is set, input contains whitespace or is not valid base64.
    if (v & 0x80808080) {
      if (!base64_decode_group_slow(dst, dstlen, src, srclen, &i, &k))
        return k;
      max_i = i + (srclen - i) / 4 * 4; // Align max_i again.
    } else {
      dst[k + 0] = (char)(((v >> 22) & 0xFC) | ((v >> 20) & 0x03));
      dst[k + 1] = (char)(((v >> 12) & 0xF0) | ((v >> 10) & 0x0F));
      dst[k + 2] = (char)(((v >> 2) & 0xC0) | ((v >> 0) & 0x3F));
      i += 4;
      k += 3;
    }
  }
  if (i < srclen && k < dstlen) {
    base64_decode_group_slow(dst, dstlen, src, srclen, &i, &k);
  }
  return k;
}


size_t base64_decode(char* const dst, const size_t dstlen,
                     const char* const src, const size_t srclen) {
  const size_t decoded_size = base64_decoded_size(src, srclen);
  return base64_decode_fast(dst, dstlen, src, srclen, decoded_size);
}

static size_t base64_encode(const char* src, size_t slen, char* dst,
                            size_t dlen) {
  // We know how much we'll write, just make sure that there's space.
  dlen = base64_encoded_size(slen);

  unsigned a;
  unsigned b;
  unsigned c;
  unsigned i;
  unsigned k;
  unsigned n;

  static const char table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  i = 0;
  k = 0;
  n = slen / 3 * 3;

  while (i < n) {
    a = src[i + 0] & 0xff;
    b = src[i + 1] & 0xff;
    c = src[i + 2] & 0xff;

    dst[k + 0] = table[a >> 2];
    dst[k + 1] = table[((a & 3) << 4) | (b >> 4)];
    dst[k + 2] = table[((b & 0x0f) << 2) | (c >> 6)];
    dst[k + 3] = table[c & 0x3f];

    i += 3;
    k += 4;
  }

  if (n != slen) {
    switch (slen - n) {
      case 1:
        a = src[i + 0] & 0xff;
        dst[k + 0] = table[a >> 2];
        dst[k + 1] = table[(a & 3) << 4];
        dst[k + 2] = '=';
        dst[k + 3] = '=';
        break;

      case 2:
        a = src[i + 0] & 0xff;
        b = src[i + 1] & 0xff;
        dst[k + 0] = table[a >> 2];
        dst[k + 1] = table[((a & 3) << 4) | (b >> 4)];
        dst[k + 2] = table[(b & 0x0f) << 2];
        dst[k + 3] = '=';
        break;
    }
  }

  return dlen;
}

#endif // SRC_BASE64_H_
