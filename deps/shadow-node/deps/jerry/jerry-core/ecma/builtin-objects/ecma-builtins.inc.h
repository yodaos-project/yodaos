/* Copyright JS Foundation and other contributors, http://js.foundation
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

/* Description of built-in objects
   in format (ECMA_BUILTIN_ID_id, object_type, prototype_id, is_extensible, is_static, underscored_id) */

/* The Object.prototype object (15.2.4) */
BUILTIN (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID__COUNT /* no prototype */,
         true,
         object_prototype,
         LIT_MAGIC_STRING_OBJECT_UL)

/* The Object object (15.2.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_OBJECT,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 object,
                 LIT_MAGIC_STRING_OBJECT_UL)

#ifndef CONFIG_DISABLE_ARRAY_BUILTIN
/* The Array.prototype object (15.4.4) */
BUILTIN (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_ARRAY,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         array_prototype,
         LIT_MAGIC_STRING_ARRAY_UL)

/* The Array object (15.4.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 array,
                 LIT_MAGIC_STRING_ARRAY_UL)
#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN*/

#ifndef CONFIG_DISABLE_STRING_BUILTIN
/* The String.prototype object (15.5.4) */
BUILTIN (ECMA_BUILTIN_ID_STRING_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         string_prototype,
         LIT_MAGIC_STRING_STRING_UL)

/* The String object (15.5.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_STRING,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 string,
                 LIT_MAGIC_STRING_STRING_UL)
#endif /* !CONFIG_DISABLE_STRING_BUILTIN */

#ifndef CONFIG_DISABLE_BOOLEAN_BUILTIN
/* The Boolean.prototype object (15.6.4) */
BUILTIN (ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         boolean_prototype,
         LIT_MAGIC_STRING_BOOLEAN_UL)

/* The Boolean object (15.6.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_BOOLEAN,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 boolean,
                 LIT_MAGIC_STRING_BOOLEAN_UL)
#endif /* !CONFIG_DISABLE_BOOLEAN_BUILTIN */

#ifndef CONFIG_DISABLE_NUMBER_BUILTIN
/* The Number.prototype object (15.7.4) */
BUILTIN (ECMA_BUILTIN_ID_NUMBER_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         number_prototype,
         LIT_MAGIC_STRING_NUMBER_UL)

/* The Number object (15.7.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_NUMBER,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 number,
                 LIT_MAGIC_STRING_NUMBER_UL)
#endif /* !CONFIG_DISABLE_NUMBER_BUILTIN */

/* The Function.prototype object (15.3.4) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
                 true,
                 function_prototype,
                 LIT_MAGIC_STRING_FUNCTION_UL)

/* The Function object (15.3.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FUNCTION,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 function,
                 LIT_MAGIC_STRING_FUNCTION_UL)

#ifndef CONFIG_DISABLE_MATH_BUILTIN
/* The Math object (15.8) */
BUILTIN (ECMA_BUILTIN_ID_MATH,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         math,
         LIT_MAGIC_STRING_MATH_UL)
#endif /* !CONFIG_DISABLE_MATH_BUILTIN */

#ifndef CONFIG_DISABLE_JSON_BUILTIN
/* The JSON object (15.12) */
BUILTIN (ECMA_BUILTIN_ID_JSON,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         json,
         LIT_MAGIC_STRING_JSON_U)
#endif /* !CONFIG_DISABLE_JSON_BUILTIN */

#ifndef CONFIG_DISABLE_DATE_BUILTIN
/* The Date.prototype object (15.9.4) */
BUILTIN (ECMA_BUILTIN_ID_DATE_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         date_prototype,
         LIT_MAGIC_STRING_DATE_UL)

/* The Date object (15.9.3) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_DATE,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 date,
                 LIT_MAGIC_STRING_DATE_UL)
#endif /* !CONFIG_DISABLE_DATE_BUILTIN */

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
/* The RegExp.prototype object (15.10.6) */
BUILTIN (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         regexp_prototype,
         LIT_MAGIC_STRING_REGEXP_UL)

/* The RegExp object (15.10) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_REGEXP,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 regexp,
                 LIT_MAGIC_STRING_REGEXP_UL)
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

/* The Error object (15.11.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 error,
                 LIT_MAGIC_STRING_ERROR_UL)

/* The Error.prototype object (15.11.4) */
BUILTIN (ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         error_prototype,
         LIT_MAGIC_STRING_ERROR_UL)

#ifndef CONFIG_DISABLE_ERROR_BUILTINS
/* The EvalError.prototype object (15.11.6.1) */
BUILTIN (ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         eval_error_prototype,
         LIT_MAGIC_STRING_EVAL_ERROR_UL)

/* The EvalError object (15.11.6.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_EVAL_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 eval_error,
                 LIT_MAGIC_STRING_EVAL_ERROR_UL)

/* The RangeError.prototype object (15.11.6.2) */
BUILTIN (ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         range_error_prototype,
         LIT_MAGIC_STRING_RANGE_ERROR_UL)

/* The RangeError object (15.11.6.2) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_RANGE_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 range_error,
                 LIT_MAGIC_STRING_RANGE_ERROR_UL)

/* The ReferenceError.prototype object (15.11.6.3) */
BUILTIN (ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         reference_error_prototype,
         LIT_MAGIC_STRING_REFERENCE_ERROR_UL)

/* The ReferenceError object (15.11.6.3) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_REFERENCE_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 reference_error,
                 LIT_MAGIC_STRING_REFERENCE_ERROR_UL)

/* The SyntaxError.prototype object (15.11.6.4) */
BUILTIN (ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         syntax_error_prototype,
         LIT_MAGIC_STRING_SYNTAX_ERROR_UL)

/* The SyntaxError object (15.11.6.4) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_SYNTAX_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 syntax_error,
                 LIT_MAGIC_STRING_SYNTAX_ERROR_UL)

/* The TypeError.prototype object (15.11.6.5) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         type_error_prototype,
         LIT_MAGIC_STRING_TYPE_ERROR_UL)

/* The TypeError object (15.11.6.5) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_TYPE_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 type_error,
                 LIT_MAGIC_STRING_TYPE_ERROR_UL)

/* The URIError.prototype object (15.11.6.6) */
BUILTIN (ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         uri_error_prototype,
         LIT_MAGIC_STRING_URI_ERROR_UL)

/* The URIError object (15.11.6.6) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_URI_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 uri_error,
                 LIT_MAGIC_STRING_URI_ERROR_UL)
#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */

/**< The [[ThrowTypeError]] object (13.2.3) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 false,
                 type_error_thrower,
                 LIT_MAGIC_STRING_TYPE_ERROR_UL)

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

/* The ArrayBuffer.prototype object (ES2015 24.1.4) */
BUILTIN (ECMA_BUILTIN_ID_ARRAYBUFFER_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         arraybuffer_prototype,
         LIT_MAGIC_STRING_ARRAY_BUFFER_UL)

/* The ArrayBuffer object (ES2015 24.1.2) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_ARRAYBUFFER,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 arraybuffer,
                 LIT_MAGIC_STRING_ARRAY_BUFFER_UL)

 /* The %TypedArrayPrototype% object (ES2015 24.2.3) */
BUILTIN (ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         typedarray_prototype,
         LIT_MAGIC_STRING_TYPED_ARRAY_UL)

/* The %TypedArray% intrinsic object (ES2015 22.2.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_TYPEDARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 typedarray,
                 LIT_MAGIC_STRING_TYPED_ARRAY_UL)

BUILTIN (ECMA_BUILTIN_ID_INT8ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         int8array_prototype,
         LIT_MAGIC_STRING_INT8_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_INT8ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 int8array,
                 LIT_MAGIC_STRING_INT8_ARRAY_UL)

BUILTIN (ECMA_BUILTIN_ID_UINT8ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint8array_prototype,
         LIT_MAGIC_STRING_UINT8_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT8ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint8array,
                 LIT_MAGIC_STRING_UINT8_ARRAY_UL)

BUILTIN (ECMA_BUILTIN_ID_INT16ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         int16array_prototype,
         LIT_MAGIC_STRING_INT16_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_INT16ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 int16array,
                 LIT_MAGIC_STRING_INT16_ARRAY_UL)

BUILTIN (ECMA_BUILTIN_ID_UINT16ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint16array_prototype,
         LIT_MAGIC_STRING_UINT16_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT16ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint16array,
                 LIT_MAGIC_STRING_UINT16_ARRAY_UL)

BUILTIN (ECMA_BUILTIN_ID_INT32ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         int32array_prototype,
         LIT_MAGIC_STRING_INT32_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_INT32ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 int32array,
                 LIT_MAGIC_STRING_INT32_ARRAY_UL)

BUILTIN (ECMA_BUILTIN_ID_UINT32ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint32array_prototype,
         LIT_MAGIC_STRING_UINT32_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT32ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint32array,
                 LIT_MAGIC_STRING_UINT32_ARRAY_UL)

BUILTIN (ECMA_BUILTIN_ID_FLOAT32ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         float32array_prototype,
         LIT_MAGIC_STRING_FLOAT32_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FLOAT32ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 float32array,
                 LIT_MAGIC_STRING_FLOAT32_ARRAY_UL)

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
BUILTIN (ECMA_BUILTIN_ID_FLOAT64ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         float64array_prototype,
         LIT_MAGIC_STRING_FLOAT64_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FLOAT64ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 float64array,
                 LIT_MAGIC_STRING_FLOAT64_ARRAY_UL)
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */

BUILTIN (ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint8clampedarray_prototype,
         LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint8clampedarray,
                 LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL)

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN

BUILTIN (ECMA_BUILTIN_ID_PROMISE_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         promise_prototype,
         LIT_MAGIC_STRING_PROMISE_UL)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_PROMISE,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 promise,
                 LIT_MAGIC_STRING_PROMISE_UL)

#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */

#ifndef CONFIG_DISABLE_ES2015_SYMBOL_BUILTIN

/* The Symbol prototype object (ECMA-262 v6, 19.4.2.7) */
BUILTIN (ECMA_BUILTIN_ID_SYMBOL_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         symbol_prototype,
         LIT_MAGIC_STRING_SYMBOL_UL)

/* The Symbol routine (ECMA-262 v6, 19.4.2.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_SYMBOL,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 symbol,
                 LIT_MAGIC_STRING_SYMBOL_UL)

#endif /* !CONFIG_DISABLE_ES2015_SYMBOL_BUILTIN */

/* The Global object (15.1) */
BUILTIN (ECMA_BUILTIN_ID_GLOBAL,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, /* Implementation-dependent */
         true,
         global,
         LIT_MAGIC_STRING_GLOBAL)

#undef BUILTIN
