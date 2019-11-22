#include "iotjs_def.h"
#include "iotjs_module_buffer.h"
#include "iotjs_objectwrap.h"
#include <mbedtls/md.h>

typedef struct {
  iotjs_jobjectwrap_t jobjectwrap;
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t* info;
} IOTJS_VALIDATED_STRUCT(iotjs_crypto_hash_t);

static iotjs_crypto_hash_t* iotjs_crypto_hash_create(const jerry_value_t jval);
static void iotjs_crypto_hash_destroy(iotjs_crypto_hash_t* wrap);

static JNativeInfoType this_module_native_info = {
  .free_cb = (jerry_object_native_free_callback_t)iotjs_crypto_hash_destroy
};

static iotjs_crypto_hash_t* iotjs_crypto_hash_create(const jerry_value_t jval) {
  iotjs_crypto_hash_t* hash = IOTJS_ALLOC(iotjs_crypto_hash_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_crypto_hash_t, hash);
  iotjs_jobjectwrap_initialize(&_this->jobjectwrap, jval,
                               &this_module_native_info);
  return hash;
}

static void iotjs_crypto_hash_destroy(iotjs_crypto_hash_t* hashwrap) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_crypto_hash_t, hashwrap);
  iotjs_jobjectwrap_destroy(&_this->jobjectwrap);
  mbedtls_md_free(&_this->ctx);
  IOTJS_RELEASE(hashwrap);
}

JS_FUNCTION(HashConstructor) {
  DJS_CHECK_THIS();

  // Create Hash object
  const jerry_value_t val = JS_GET_THIS();
  iotjs_crypto_hash_t* hash = iotjs_crypto_hash_create(val);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_crypto_hash_t, hash);

  mbedtls_md_init(&_this->ctx);
  size_t type = jerry_get_number_value(jargv[0]);
  _this->info = mbedtls_md_info_from_type(type);

  int r = mbedtls_md_init_ctx(&_this->ctx, _this->info);
  if (r != 0) {
    return JS_CREATE_ERROR(COMMON, "md_init_ctx() failed");
  }
  mbedtls_md_starts(&_this->ctx);
  return jerry_create_undefined();
}

JS_FUNCTION(HashUpdate) {
  JS_DECLARE_THIS_PTR(crypto_hash, hash);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_crypto_hash_t, hash);

  jerry_value_t jval = jargv[0];
  iotjs_bufferwrap_t* buf_wrap = iotjs_bufferwrap_from_jbuffer(jval);
  size_t size = iotjs_bufferwrap_length(buf_wrap);
  char* buf = iotjs_bufferwrap_buffer(buf_wrap);
  mbedtls_md_update(&_this->ctx, (unsigned char*)buf, size);
  return jerry_create_null();
}

JS_FUNCTION(HashDigest) {
  JS_DECLARE_THIS_PTR(crypto_hash, hash);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_crypto_hash_t, hash);

  size_t size = MBEDTLS_MD_MAX_SIZE;
  unsigned char contents[size];
  mbedtls_md_finish(&_this->ctx, contents);

  size = mbedtls_md_get_size(_this->info);
  mbedtls_md_free(&_this->ctx);

  jerry_value_t jbuffer = iotjs_bufferwrap_create_buffer(size);
  iotjs_bufferwrap_t* buffer_wrap = iotjs_bufferwrap_from_jbuffer(jbuffer);
  iotjs_bufferwrap_copy(buffer_wrap, (char*)contents, size);
  return jbuffer;
}

jerry_value_t InitCryptoHash() {
  jerry_value_t exports = jerry_create_object();
  jerry_value_t hashConstructor =
      jerry_create_external_function(HashConstructor);
  iotjs_jval_set_property_jval(exports, "Hash", hashConstructor);

  jerry_value_t proto = jerry_create_object();
  iotjs_jval_set_method(proto, "update", HashUpdate);
  iotjs_jval_set_method(proto, "digest", HashDigest);
  iotjs_jval_set_property_jval(hashConstructor, "prototype", proto);

  jerry_release_value(proto);
  jerry_release_value(hashConstructor);
  return exports;
}
