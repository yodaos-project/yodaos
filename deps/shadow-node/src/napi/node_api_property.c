/* Copyright 2018-present Rokid Co., Ltd. and other contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jerryscript-ext/handle-scope.h"
#include "jerryscript.h"
#include "internal/node_api_internal.h"

napi_status napi_get_property_names(napi_env env, napi_value object,
                                    napi_value* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval = AS_JERRY_VALUE(object);
  NAPI_TRY_TYPE(object, jval);

  jerry_value_t jval_keys = jerry_get_object_keys(jval);
  jerryx_create_handle(jval_keys);
  if (jerry_value_has_error_flag(jval_keys))
    NAPI_RETURN(napi_invalid_arg, NULL);

  NAPI_ASSIGN(result, AS_NAPI_VALUE(jval_keys));
  NAPI_RETURN(napi_ok);
}

napi_status napi_set_property(napi_env env, napi_value object, napi_value key,
                              napi_value value) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  jerry_value_t jval_key = AS_JERRY_VALUE(key);
  jerry_value_t jval_val = AS_JERRY_VALUE(value);

  NAPI_TRY_TYPE(object, jval_object);

  jerry_value_t ret = jerry_set_property(jval_object, jval_key, jval_val);
  if (jerry_value_has_error_flag(ret)) {
    jerry_release_value(ret);
    NAPI_RETURN(napi_invalid_arg, NULL);
  }

  jerry_release_value(ret);
  NAPI_RETURN(napi_ok);
}

napi_status napi_get_property(napi_env env, napi_value object, napi_value key,
                              napi_value* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  jerry_value_t jval_key = AS_JERRY_VALUE(key);

  NAPI_TRY_TYPE(object, jval_object);

  jerry_value_t jval_ret = jerry_get_property(jval_object, jval_key);
  jerryx_create_handle(jval_ret);
  if (jerry_value_has_error_flag(jval_ret))
    NAPI_RETURN(napi_invalid_arg, NULL);

  NAPI_ASSIGN(result, AS_NAPI_VALUE(jval_ret));
  NAPI_RETURN(napi_ok);
}

napi_status napi_has_property(napi_env env, napi_value object, napi_value key,
                              bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  jerry_value_t jval_key = AS_JERRY_VALUE(key);

  NAPI_TRY_TYPE(object, jval_object);

  NAPI_ASSIGN(result, jerry_has_property(jval_object, jval_key));

  NAPI_RETURN(napi_ok);
}

napi_status napi_delete_property(napi_env env, napi_value object,
                                 napi_value key, bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  jerry_value_t jval_key = AS_JERRY_VALUE(key);

  NAPI_TRY_TYPE(object, jval_object);

  NAPI_ASSIGN(result, jerry_delete_property(jval_object, jval_key));
  NAPI_RETURN(napi_ok);
}

napi_status napi_has_own_property(napi_env env, napi_value object,
                                  napi_value key, bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  jerry_value_t jval_key = AS_JERRY_VALUE(key);

  NAPI_TRY_TYPE(object, jval_object);

  NAPI_ASSIGN(result, jerry_has_own_property(jval_object, jval_key));
  NAPI_RETURN(napi_ok);
}

napi_status napi_set_named_property(napi_env env, napi_value object,
                                    const char* utf8Name, napi_value value) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  NAPI_TRY_TYPE(object, jval_object);

  jerry_value_t jval_key =
      jerry_create_string_from_utf8((jerry_char_t*)utf8Name);
  napi_status status =
      napi_set_property(env, object, AS_NAPI_VALUE(jval_key), value);
  jerry_release_value(jval_key);
  return status;
}

napi_status napi_get_named_property(napi_env env, napi_value object,
                                    const char* utf8Name, napi_value* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  NAPI_TRY_TYPE(object, jval_object);

  jerry_value_t jval_key =
      jerry_create_string_from_utf8((jerry_char_t*)utf8Name);
  napi_status status =
      napi_get_property(env, object, AS_NAPI_VALUE(jval_key), result);
  jerry_release_value(jval_key);
  return status;
}

napi_status napi_has_named_property(napi_env env, napi_value object,
                                    const char* utf8Name, bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  NAPI_TRY_TYPE(object, jval_object);

  jerry_value_t jval_key =
      jerry_create_string_from_utf8((jerry_char_t*)utf8Name);
  napi_status status =
      napi_has_property(env, object, AS_NAPI_VALUE(jval_key), result);
  jerry_release_value(jval_key);
  return status;
}

napi_status napi_set_element(napi_env env, napi_value object, uint32_t index,
                             napi_value value) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  jerry_value_t jval_val = AS_JERRY_VALUE(value);

  NAPI_TRY_TYPE(object, jval_object);

  jerry_value_t res = jerry_set_property_by_index(jval_object, index, jval_val);
  if (jerry_value_has_error_flag(res)) {
    jerry_release_value(res);
    NAPI_RETURN(napi_invalid_arg, NULL);
  }

  jerry_release_value(res);
  NAPI_RETURN(napi_ok);
}

napi_status napi_get_element(napi_env env, napi_value object, uint32_t index,
                             napi_value* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);

  NAPI_TRY_TYPE(object, jval_object);

  jerry_value_t jval_ret = jerry_get_property_by_index(jval_object, index);
  jerryx_create_handle(jval_ret);
  if (jerry_value_has_error_flag(jval_ret)) {
    NAPI_RETURN(napi_invalid_arg, NULL);
  }
  NAPI_ASSIGN(result, AS_NAPI_VALUE(jval_ret));
  NAPI_RETURN(napi_ok);
}

napi_status napi_has_element(napi_env env, napi_value object, uint32_t index,
                             bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  NAPI_TRY_TYPE(object, jval_object);

  NAPI_ASSIGN(result, jerry_has_property_by_index(jval_object, index));
  NAPI_RETURN(napi_ok);
}

napi_status napi_delete_element(napi_env env, napi_value object, uint32_t index,
                                bool* result) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_object = AS_JERRY_VALUE(object);
  NAPI_TRY_TYPE(object, jval_object);

  NAPI_ASSIGN(result, jerry_delete_property_by_index(jval_object, index));
  NAPI_RETURN(napi_ok);
}

napi_status iotjs_napi_prop_desc_to_jdesc(napi_env env,
                                          const napi_property_descriptor* ndesc,
                                          jerry_property_descriptor_t* jdesc) {
  napi_status status;
#define JATTR_FROM_NAPI_ATTR(prop_attr)       \
  if (ndesc->attributes & napi_##prop_attr) { \
    jdesc->is_##prop_attr##_defined = true;   \
    jdesc->is_##prop_attr = true;             \
  }

  JATTR_FROM_NAPI_ATTR(writable);
  JATTR_FROM_NAPI_ATTR(enumerable);
  JATTR_FROM_NAPI_ATTR(configurable);

#undef JATTR_FROM_NAPI_ATTR

  if (ndesc->value != NULL) {
    jdesc->is_value_defined = true;
    jdesc->value = AS_JERRY_VALUE(ndesc->value);
    NAPI_RETURN(napi_ok);
  }

  if (ndesc->method != NULL) {
    napi_value method;
    status = napi_create_function(env, "method", 6, ndesc->method, ndesc->data,
                                  &method);
    if (status != napi_ok)
      return status;
    jdesc->is_value_defined = true;
    jdesc->value = AS_JERRY_VALUE(method);
    NAPI_RETURN(napi_ok);
  }

#define JACCESSOR_FROM_NAPI_ATTR(access)                                       \
  do {                                                                         \
    if (ndesc->access##ter != NULL) {                                          \
      napi_value access##ter;                                                  \
      status = napi_create_function(env, #access "ter", 6, ndesc->access##ter, \
                                    ndesc->data, &access##ter);                \
      if (status != napi_ok)                                                   \
        return status;                                                         \
      jdesc->is_##access##_defined = true;                                     \
      jdesc->access##ter = AS_JERRY_VALUE(access##ter);                        \
      /** jerryscript asserts xor is_writable_defined and accessors */         \
      jdesc->is_writable_defined = false;                                      \
    }                                                                          \
  } while (0)

  JACCESSOR_FROM_NAPI_ATTR(get);
  JACCESSOR_FROM_NAPI_ATTR(set);

#undef JACCESSOR_FROM_NAPI_ATTR

  NAPI_RETURN(napi_ok);
}

napi_status napi_define_properties(napi_env env, napi_value object,
                                   size_t property_count,
                                   const napi_property_descriptor* properties) {
  NAPI_TRY_ENV(env);
  jerry_value_t jval_target = AS_JERRY_VALUE(object);
  NAPI_TRY_TYPE(object, jval_target);
  NAPI_WEAK_ASSERT(napi_invalid_arg, properties != NULL);

  napi_status status;
  jerry_property_descriptor_t prop_desc;
  for (size_t i = 0; i < property_count; ++i) {
    jerry_init_property_descriptor_fields(&prop_desc);
    napi_property_descriptor prop = properties[i];

    jerry_value_t jval_prop_name;
    if (prop.utf8name != NULL) {
      jval_prop_name =
          jerry_create_string_from_utf8((jerry_char_t*)prop.utf8name);
      jerryx_create_handle(jval_prop_name);
    } else if (prop.name != NULL) {
      jval_prop_name = AS_JERRY_VALUE(prop.name);
    } else {
      NAPI_RETURN(napi_invalid_arg, NULL);
    }

    status = iotjs_napi_prop_desc_to_jdesc(env, &prop, &prop_desc);
    if (status != napi_ok)
      return status;

    jerry_value_t return_value =
        jerry_define_own_property(jval_target, jval_prop_name, &prop_desc);
    if (jerry_value_has_error_flag(return_value)) {
      NAPI_RETURN(napi_invalid_arg, NULL);
    }
    jerry_release_value(return_value);

    /**
     * We don't have to call `jerry_free_property_descriptor_fields`
     * since most napi values are managed by handle scopes.
     */
    // jerry_free_property_descriptor_fields(&prop_desc);
  }

  NAPI_RETURN(napi_ok);
}
