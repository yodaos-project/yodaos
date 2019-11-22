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

#include <math.h>
#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-number-object.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_NUMBER_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-number.inc.h"
#define BUILTIN_UNDERSCORED_ID number
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup number ECMA Number object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in Number object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_number_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  if (arguments_list_len == 0)
  {
    ret_value = ecma_make_integer_value (0);
  }
  else
  {
    ret_value = ecma_op_to_number (arguments_list_p[0]);
  }

  return ret_value;
} /* ecma_builtin_number_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Number object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_number_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len == 0)
  {
    ecma_value_t completion = ecma_op_create_number_object (ecma_make_integer_value (0));
    return completion;
  }
  else
  {
    return ecma_op_create_number_object (arguments_list_p[0]);
  }
} /* ecma_builtin_number_dispatch_construct */

/**
 * The Number object's 'isInteger' routine
 *
 * See also:
 *          ECMA-262, 20.1.2.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_object_is_integer (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t arg) /**< routine's first argument */
{
    JERRY_UNUSED (this_arg);
    if (!ecma_is_value_number (arg))
    {
        return ECMA_VALUE_FALSE;
    }

    ecma_value_t ret_value = ECMA_VALUE_EMPTY;
    ECMA_OP_TO_NUMBER_TRY_CATCH(arg_num, arg, ret_value) ;

    ret_value = !ecma_number_is_nan (arg_num) &&
                !ecma_number_is_infinity (arg_num) &&
                floor (arg_num) == arg_num;

    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
    return ret_value ? ECMA_VALUE_TRUE : ECMA_VALUE_FALSE;
} /* ecma_builtin_number_object_is_integer */

/**
 * The Number object's 'isSafeInteger' routine
 *
 * See also:
 *          ECMA-262, 20.1.2.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_object_is_safe_integer (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg) /**< routine's first argument */
{
    JERRY_UNUSED (this_arg);
    ecma_value_t is_integer = ecma_builtin_number_object_is_integer (this_arg, arg);
    if (!ecma_op_to_boolean (is_integer))
    {
        return ECMA_VALUE_FALSE;
    }

    ecma_number_t arg_num = ecma_get_number_from_value (arg);
    return fabs (arg_num) <= 9007199254740991 ? ECMA_VALUE_TRUE : ECMA_VALUE_FALSE;
} /* ecma_builtin_number_object_is_safe_integer */

/**
 * The Number object's 'isFinite' routine
 *
 * See also:
 *          ECMA-262, 20.1.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_object_is_finite(ecma_value_t this_arg, /**< this argument */
                                     ecma_value_t arg) /**< routine's first argument */
{
    JERRY_UNUSED (this_arg);
    if (!ecma_is_value_number (arg))
    {
        return ECMA_VALUE_FALSE;
    }

    ecma_value_t ret_value = ECMA_VALUE_EMPTY;
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value) ;

    bool is_finite = !ecma_number_is_nan (arg_num) && !ecma_number_is_infinity (arg_num);
    ret_value = is_finite ? ECMA_VALUE_TRUE : ECMA_VALUE_FALSE;

    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
    return ret_value;
} /* ecma_builtin_number_object_is_finite */

/**
 * The Number object's 'isNaN' routine
 *
 * See also:
 *          ECMA-262, 20.1.2.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_object_is_nan (ecma_value_t this_arg, /**< this argument */
                                   ecma_value_t arg) /**< routine's first argument */
{
    JERRY_UNUSED(this_arg);
    if (!ecma_is_value_number (arg))
    {
        return ECMA_VALUE_FALSE;
    }

    ecma_value_t ret_value = ECMA_VALUE_EMPTY;
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value) ;

    bool is_nan = ecma_number_is_nan (arg_num);
    ret_value = is_nan ? ECMA_VALUE_TRUE : ECMA_VALUE_FALSE;

    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
    return ret_value;
} /* ecma_builtin_number_object_is_nan */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_NUMBER_BUILTIN */
