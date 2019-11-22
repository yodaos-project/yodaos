#include "iotjs_def.h"
#include "iotjs_module_buffer.h"
#include "iotjs_objectwrap.h"
#include <stdlib.h>
#include <sys/types.h>
#include "zlib.h"

// Custom constants used by both node_constants.cc and node_zlib.cc
#define Z_MIN_WINDOWBITS 8
#define Z_MAX_WINDOWBITS 15
#define Z_DEFAULT_WINDOWBITS 15
// Fewer than 64 bytes per chunk is not recommended.
// Technically it could work with as few as 8, but even 64 bytes
// is low.  Usually a MB or more is best.
#define Z_MIN_CHUNK 64
#define Z_MAX_CHUNK 65535
#define Z_DEFAULT_CHUNK (16 * 1024)
#define Z_MIN_MEMLEVEL 1
#define Z_MAX_MEMLEVEL 9
#define Z_DEFAULT_MEMLEVEL 8
#define Z_MIN_LEVEL -1
#define Z_MAX_LEVEL 9
#define Z_DEFAULT_LEVEL Z_DEFAULT_COMPRESSION

#define GZIP_HEADER_ID1 0x1f
#define GZIP_HEADER_ID2 0x8b

enum node_zlib_mode {
  NONE,
  DEFLATE,
  INFLATE,
  GZIP,
  GUNZIP,
  DEFLATERAW,
  INFLATERAW,
  UNZIP
};

typedef struct {
  iotjs_jobjectwrap_t jobjectwrap;
  Bytef* dictionary_;
  size_t dictionary_len_;
  int err_;
  int flush_;
  bool init_done_;
  int level_;
  int memLevel_;
  enum node_zlib_mode mode_;
  int strategy_;
  z_stream strm_;
  int windowBits_;
  uv_work_t work_req_;
  bool write_in_progress_;
  bool pending_close_;
  unsigned int refs_;
  unsigned int gzip_id_bytes_read_;
  unsigned char* out_;
  jerry_value_t write_result_;
  jerry_value_t write_callback_;
  jerry_value_t out_buf_;
} IOTJS_VALIDATED_STRUCT(iotjs_zlib_t);

static JNativeInfoType this_module_native_info = { .free_cb = NULL };

static iotjs_zlib_t* iotjs_zlib_create(const jerry_value_t jval,
                                       jerry_value_t mode) {
  iotjs_zlib_t* zlib = IOTJS_ALLOC(iotjs_zlib_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_zlib_t, zlib);
  iotjs_jobjectwrap_initialize(&_this->jobjectwrap, jval,
                               &this_module_native_info);

  _this->dictionary_ = NULL;
  _this->dictionary_len_ = 0;
  _this->err_ = 0;
  _this->flush_ = 0;
  _this->init_done_ = false;
  _this->level_ = 0;
  _this->memLevel_ = 0;
  _this->mode_ = jerry_get_number_value(mode);
  _this->strategy_ = 0;
  _this->windowBits_ = 0;
  _this->write_in_progress_ = false;
  _this->pending_close_ = false;
  _this->refs_ = 0;
  _this->gzip_id_bytes_read_ = 0;
  return zlib;
}

static void iotjs_zlib_reset(iotjs_zlib_t_impl_t* _this) {
  _this->err_ = Z_OK;
  switch (_this->mode_) {
    case DEFLATE:
    case DEFLATERAW:
    case GZIP:
      _this->err_ = deflateReset(&_this->strm_);
      break;
    case INFLATE:
    case INFLATERAW:
    case GUNZIP:
      _this->err_ = inflateReset(&_this->strm_);
      break;
    default:
      break;
  }

  if (_this->err_ != Z_OK) {
    fprintf(stderr, "Failed to reset stream\n");
  }
}

static void iotjs_zlib_set_dir(iotjs_zlib_t_impl_t* _this) {
  if (_this->dictionary_ == NULL)
    return;

  _this->err_ = Z_OK;
  switch (_this->mode_) {
    case DEFLATE:
    case DEFLATERAW:
      _this->err_ = deflateSetDictionary(&_this->strm_, _this->dictionary_,
                                         _this->dictionary_len_);
      break;
    case INFLATERAW:
      // The other inflate cases will have the dictionary set when inflate()
      // returns Z_NEED_DICT in Process()
      _this->err_ = inflateSetDictionary(&_this->strm_, _this->dictionary_,
                                         _this->dictionary_len_);
      break;
    default:
      break;
  }
  if (_this->err_ != Z_OK) {
    fprintf(stderr, "Failed to set dictionary\n");
  }
}


static void iotjs_zlib_process(uv_work_t* work_req) {
  iotjs_zlib_t_impl_t* _this = (iotjs_zlib_t_impl_t*)(work_req->data);
  _this->err_ = 0;

  const Bytef* next_expected_header_byte = NULL;

  // If the avail_out is left at 0, then it means that it ran out
  // of room.  If there was avail_out left over, then it means
  // that all of the input was consumed.
  switch (_this->mode_) {
    case DEFLATE:
    case GZIP:
    case DEFLATERAW:
      _this->err_ = deflate(&_this->strm_, _this->flush_);
      break;
    case UNZIP:
      if (_this->strm_.avail_in > 0) {
        next_expected_header_byte = _this->strm_.next_in;
      }
      switch (_this->gzip_id_bytes_read_) {
        case 0:
          if (next_expected_header_byte == NULL) {
            break;
          }

          if (*next_expected_header_byte == GZIP_HEADER_ID1) {
            _this->gzip_id_bytes_read_ = 1;
            next_expected_header_byte++;

            if (_this->strm_.avail_in == 1) {
              // The only available byte was already read.
              break;
            }
          } else {
            _this->mode_ = INFLATE;
            break;
          }

          // fallthrough
        case 1:
          if (next_expected_header_byte == NULL) {
            break;
          }

          if (*next_expected_header_byte == GZIP_HEADER_ID2) {
            _this->gzip_id_bytes_read_ = 2;
            _this->mode_ = GUNZIP;
          } else {
            // There is no actual difference between INFLATE and INFLATERAW
            // (after initialization).
            _this->mode_ = INFLATE;
          }

          break;
        default:
          JS_CREATE_ERROR(COMMON,
                          "invalid number of gzip magic number bytes read");
          return;
      }

      // fallthrough
    case INFLATE:
    case GUNZIP:
    case INFLATERAW:
      _this->err_ = inflate(&_this->strm_, _this->flush_);

      // If data was encoded with dictionary (INFLATERAW will have it set in
      // SetDictionary, don't repeat that here)
      if (_this->mode_ != INFLATERAW && _this->err_ == Z_NEED_DICT &&
          _this->dictionary_ != NULL) {
        // Load it
        _this->err_ = inflateSetDictionary(&_this->strm_, _this->dictionary_,
                                           _this->dictionary_len_);
        if (_this->err_ == Z_OK) {
          // And try to decode again
          _this->err_ = inflate(&_this->strm_, _this->flush_);
        } else if (_this->err_ == Z_DATA_ERROR) {
          // Both inflateSetDictionary() and inflate() return Z_DATA_ERROR.
          // Make it possible for After() to tell a bad dictionary from bad
          // input.
          _this->err_ = Z_NEED_DICT;
        }
      }

      while (_this->strm_.avail_in > 0 && _this->mode_ == GUNZIP &&
             _this->err_ == Z_STREAM_END && _this->strm_.next_in[0] != 0x00) {
        // Bytes remain in input buffer. Perhaps this is another compressed
        // member in the same archive, or just trailing garbage.
        // Trailing zero bytes are okay, though, since they are frequently
        // used for padding.

        iotjs_zlib_reset(_this);
        iotjs_zlib_set_dir(_this);
        _this->err_ = inflate(&_this->strm_, _this->flush_);
      }
      break;
    default:
      break;
  }

  // pass any errors back to the main thread to deal with.

  // now After will emit the output, and
  // either schedule another call to Process,
  // or shift the queue and call Process.
}

static void iotjs_zlib_after_process(uv_work_t* work_req, int status) {
  iotjs_zlib_t_impl_t* _this = (iotjs_zlib_t_impl_t*)(work_req->data);
  jerry_value_t jthis = iotjs_jobjectwrap_jobject(&_this->jobjectwrap);

  iotjs_bufferwrap_t* out_buf = iotjs_bufferwrap_from_jbuffer(_this->out_buf_);
  iotjs_bufferwrap_copy(out_buf, (char*)_this->out_, _this->strm_.avail_out);

  if (_this->out_ != NULL)
    free(_this->out_);

  jerry_set_property_by_index(_this->write_result_, 0,
                              jerry_create_number(_this->strm_.avail_out));
  jerry_set_property_by_index(_this->write_result_, 1,
                              jerry_create_number(_this->strm_.avail_in));
  _this->write_in_progress_ = false;

  iotjs_jargs_t jargs = iotjs_jargs_create(0);
  iotjs_make_callback(_this->write_callback_, jthis, &jargs);
}

JS_FUNCTION(ZlibConstructor) {
  DJS_CHECK_THIS();
  const jerry_value_t val = JS_GET_THIS();
  iotjs_zlib_create(val, jargv[0]);
  return jerry_create_undefined();
}

JS_FUNCTION(ZlibInit) {
  JS_DECLARE_THIS_PTR(zlib, zlib);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_zlib_t, zlib);

  if (jargc == 5) {
    fprintf(stderr,
            "WARNING: You are likely using a version of node-tar or npm that "
            "is incompatible with this version of Node.js.\nPlease use "
            "either the version of npm that is bundled with Node.js, or "
            "a version of npm (> 5.5.1 or < 5.4.0) or node-tar (> 4.0.1) "
            "that is compatible with Node.js 9 and above.\n");
  }

  int windowBits = jerry_get_number_value(jargv[0]);
  if (windowBits < Z_MIN_WINDOWBITS || windowBits > Z_MAX_WINDOWBITS) {
    return JS_CREATE_ERROR(COMMON, "invalid windowBits");
  }

  int level = jerry_get_number_value(jargv[1]);
  if (level < Z_MIN_LEVEL || level > Z_MAX_LEVEL) {
    return JS_CREATE_ERROR(COMMON, "invalid compression level");
  }

  int memLevel = jerry_get_number_value(jargv[2]);
  if (memLevel < Z_MIN_MEMLEVEL || memLevel > Z_MAX_MEMLEVEL) {
    return JS_CREATE_ERROR(COMMON, "invalid memlevel");
  }

  int strategy = jerry_get_number_value(jargv[3]);
  if (strategy != Z_FILTERED && strategy != Z_HUFFMAN_ONLY &&
      strategy != Z_RLE && strategy != Z_FIXED &&
      strategy != Z_DEFAULT_STRATEGY) {
    return JS_CREATE_ERROR(COMMON, "invalid strategy");
  }

  // write callback
  _this->write_result_ = jargv[4];
  _this->write_callback_ = jargv[5];

  // flags
  _this->level_ = level;
  _this->windowBits_ = windowBits;
  _this->memLevel_ = memLevel;
  _this->strategy_ = strategy;
  _this->strm_.zalloc = Z_NULL;
  _this->strm_.zfree = Z_NULL;
  _this->strm_.opaque = Z_NULL;
  _this->flush_ = Z_NO_FLUSH;
  _this->err_ = Z_OK;

  if (_this->mode_ == GZIP || _this->mode_ == GUNZIP) {
    _this->windowBits_ += 16;
  }
  if (_this->mode_ == UNZIP) {
    _this->windowBits_ += 32;
  }
  if (_this->mode_ == DEFLATERAW || _this->mode_ == INFLATERAW) {
    _this->windowBits_ *= -1;
  }

  switch (_this->mode_) {
    case DEFLATE:
    case GZIP:
    case DEFLATERAW:
      _this->err_ =
          deflateInit2(&_this->strm_, _this->level_, Z_DEFLATED,
                       _this->windowBits_, _this->memLevel_, _this->strategy_);
      break;
    case INFLATE:
    case GUNZIP:
    case INFLATERAW:
    case UNZIP:
      _this->err_ = inflateInit2(&_this->strm_, _this->windowBits_);
      break;
    default:
      _this->mode_ = NONE;
      return jerry_create_boolean(false);
  }

  _this->dictionary_ = NULL;
  _this->dictionary_len_ = 0;
  _this->write_in_progress_ = false;
  _this->init_done_ = true;

  if (_this->err_ != Z_OK) {
    _this->mode_ = NONE;
    return jerry_create_boolean(false);
  }
  return jerry_create_boolean(true);
}

JS_FUNCTION(ZlibWrite) {
  JS_DECLARE_THIS_PTR(zlib, zlib);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_zlib_t, zlib);

  int flush = jerry_get_number_value(jargv[0]);
  if (flush != Z_NO_FLUSH && flush != Z_PARTIAL_FLUSH &&
      flush != Z_SYNC_FLUSH && flush != Z_FULL_FLUSH && flush != Z_FINISH &&
      flush != Z_BLOCK) {
    return JS_CREATE_ERROR(COMMON, "Invalid flush value");
  }

  Bytef* in;
  Bytef* out;
  size_t in_off, in_len, out_off, out_len;

  if (jerry_value_is_null(jargv[1])) {
    in = NULL;
    in_off = 0;
    in_len = 0;
  } else {
    iotjs_bufferwrap_t* in_buf = iotjs_bufferwrap_from_jbuffer(jargv[1]);
    in_off = jerry_get_number_value(jargv[2]);
    in_len = jerry_get_number_value(jargv[3]);

    char* contents = (char*)iotjs_bufferwrap_buffer(in_buf);
    in = (Bytef*)(contents + in_off);
  }

  out_off = jerry_get_number_value(jargv[5]);
  out_len = jerry_get_number_value(jargv[6]);
  {
    unsigned char* contents = (unsigned char*)malloc(out_len);
    if (!contents) {
      return JS_CREATE_ERROR(COMMON, "Out of Memory");
    }
    _this->out_buf_ = jargv[4];
    _this->out_ = contents;
    memset(_this->out_, 0, out_len);
    out = (Bytef*)(_this->out_ + out_off);
  }

  _this->strm_.avail_in = in_len;
  _this->strm_.next_in = in;
  _this->strm_.avail_out = out_len;
  _this->strm_.next_out = out;
  _this->flush_ = flush;
  return jerry_create_null();
}

JS_FUNCTION(ZlibDoWrite) {
  JS_DECLARE_THIS_PTR(zlib, zlib);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_zlib_t, zlib);

  uv_work_t* work_req = &(_this->work_req_);
  work_req->data = _this;
  uv_loop_t* loop = iotjs_environment_loop(iotjs_environment_get());
  uv_queue_work(loop, work_req, iotjs_zlib_process, iotjs_zlib_after_process);
  return jerry_create_null();
}

JS_FUNCTION(ZlibDoWriteSync) {
  JS_DECLARE_THIS_PTR(zlib, zlib);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_zlib_t, zlib);

  uv_work_t* work_req = &(_this->work_req_);
  work_req->data = _this;
  iotjs_zlib_process(work_req);
  return jerry_create_null();
}

JS_FUNCTION(ZlibReset) {
  JS_DECLARE_THIS_PTR(zlib, zlib);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_zlib_t, zlib);

  _this->err_ = Z_OK;
  switch (_this->mode_) {
    case DEFLATE:
    case DEFLATERAW:
    case GZIP:
      _this->err_ = deflateReset(&_this->strm_);
      break;
    case INFLATE:
    case INFLATERAW:
    case GUNZIP:
      _this->err_ = inflateReset(&_this->strm_);
      break;
    default:
      break;
  }
  if (_this->err_ != Z_OK) {
    return JS_CREATE_ERROR(COMMON, "Failed to reset stream");
  } else {
    return jerry_create_boolean(true);
  }
}

JS_FUNCTION(ZlibClose) {
  JS_DECLARE_THIS_PTR(zlib, zlib);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_zlib_t, zlib);

  if (_this->write_in_progress_) {
    _this->pending_close_ = true;
    return jerry_create_boolean(false);
  }

  _this->pending_close_ = false;
  int status = Z_OK;
  if (_this->mode_ == DEFLATE || _this->mode_ == GZIP ||
      _this->mode_ == DEFLATERAW) {
    status = deflateEnd(&_this->strm_);
  } else if (_this->mode_ == INFLATE || _this->mode_ == GUNZIP ||
             _this->mode_ == INFLATERAW || _this->mode_ == UNZIP) {
    status = inflateEnd(&_this->strm_);
  }

  if (status != Z_OK && status != Z_DATA_ERROR) {
    return JS_CREATE_ERROR(COMMON, "`status` should be Z_OK or Z_DATA_ERROR");
  }
  _this->mode_ = NONE;

  if (_this->dictionary_ != NULL) {
    free(_this->dictionary_);
    _this->dictionary_ = NULL;
  }
  return jerry_create_boolean(true);
}

jerry_value_t InitZlib() {
  jerry_value_t exports = jerry_create_object();
#define IOTJS_DEFINE_ZLIB_CONSTANTS(name)                 \
  do {                                                    \
    iotjs_jval_set_property_number(exports, #name, name); \
  } while (0)

  // states
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_NO_FLUSH);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_PARTIAL_FLUSH);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_SYNC_FLUSH);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_FULL_FLUSH);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_FINISH);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_BLOCK);

  // return/error codes
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_OK);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_STREAM_END);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_NEED_DICT);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_ERRNO);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_STREAM_ERROR);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_DATA_ERROR);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MEM_ERROR);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_BUF_ERROR);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_VERSION_ERROR);

  // flags
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_NO_COMPRESSION);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_BEST_SPEED);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_BEST_COMPRESSION);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_DEFAULT_COMPRESSION);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_FILTERED);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_HUFFMAN_ONLY);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_RLE);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_FIXED);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_DEFAULT_STRATEGY);
  IOTJS_DEFINE_ZLIB_CONSTANTS(ZLIB_VERNUM);

  // modes
  IOTJS_DEFINE_ZLIB_CONSTANTS(DEFLATE);
  IOTJS_DEFINE_ZLIB_CONSTANTS(INFLATE);
  IOTJS_DEFINE_ZLIB_CONSTANTS(GZIP);
  IOTJS_DEFINE_ZLIB_CONSTANTS(GUNZIP);
  IOTJS_DEFINE_ZLIB_CONSTANTS(DEFLATERAW);
  IOTJS_DEFINE_ZLIB_CONSTANTS(INFLATERAW);
  IOTJS_DEFINE_ZLIB_CONSTANTS(UNZIP);

  // params
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MIN_WINDOWBITS);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MAX_WINDOWBITS);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_DEFAULT_WINDOWBITS);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MIN_CHUNK);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MAX_CHUNK);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_DEFAULT_CHUNK);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MIN_MEMLEVEL);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MAX_MEMLEVEL);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_DEFAULT_MEMLEVEL);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MIN_LEVEL);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_MAX_LEVEL);
  IOTJS_DEFINE_ZLIB_CONSTANTS(Z_DEFAULT_LEVEL);
#undef IOTJS_DEFINE_ZLIB_CONSTANTS

  jerry_value_t zlibConstructor =
      jerry_create_external_function(ZlibConstructor);
  iotjs_jval_set_property_jval(exports, "Zlib", zlibConstructor);

  jerry_value_t proto = jerry_create_object();
  iotjs_jval_set_method(proto, "init", ZlibInit);
  iotjs_jval_set_method(proto, "write", ZlibWrite);
  iotjs_jval_set_method(proto, "reset", ZlibReset);
  iotjs_jval_set_method(proto, "close", ZlibClose);
  iotjs_jval_set_method(proto, "_doWrite", ZlibDoWrite);
  iotjs_jval_set_method(proto, "_doWriteSync", ZlibDoWriteSync);
  iotjs_jval_set_property_jval(zlibConstructor, "prototype", proto);

  jerry_release_value(proto);
  jerry_release_value(zlibConstructor);
  return exports;
}
