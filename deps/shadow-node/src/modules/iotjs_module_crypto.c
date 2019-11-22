#include "iotjs_def.h"
#include "iotjs_module_crypto.h"
#include "iotjs_objectwrap.h"

#define CRYPTO_DRGB_PERSONAL_KEY "0d8958fffc9ac7845e9ef1e38f606edd"

JS_FUNCTION(RandomBytes) {
  // TODO
  return jerry_create_null();
}


JS_FUNCTION(RandomBytesSync) {
  size_t size = jerry_get_number_value(jargv[0]);
  unsigned char bytes[size];
  memset(bytes, 0, size);

  mbedtls_ctr_drbg_random(&drgb_ctx, bytes, size);
  jerry_value_t res = jerry_create_array(size);

  for (size_t i = 0; i < size; i++) {
    jerry_set_property_by_index(res, i, jerry_create_number(bytes[i]));
  }
  return res;
}


jerry_value_t InitCrypto() {
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&drgb_ctx);
  mbedtls_ctr_drbg_seed(&drgb_ctx, mbedtls_entropy_func, &entropy,
                        (const unsigned char*)CRYPTO_DRGB_PERSONAL_KEY,
                        strlen(CRYPTO_DRGB_PERSONAL_KEY));

  jerry_value_t crypto = jerry_create_object();
  iotjs_jval_set_method(crypto, "randomBytes", RandomBytes);
  iotjs_jval_set_method(crypto, "randomBytesSync", RandomBytesSync);
  return crypto;
}
