#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context drgb_ctx;
