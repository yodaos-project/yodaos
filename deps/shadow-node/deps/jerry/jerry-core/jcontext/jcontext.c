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

#include "jcontext.h"
#include "ecma-helpers.h"
#include "ecma-array-object.h"

/** \addtogroup context Context
 * @{
 */

#ifndef JERRY_ENABLE_EXTERNAL_CONTEXT
/**
 * Global context.
 */
jerry_context_t jerry_global_context;

/**
 * Jerry global heap section attribute.
 */
#ifndef JERRY_HEAP_SECTION_ATTR
#define JERRY_GLOBAL_HEAP_SECTION
#else /* JERRY_HEAP_SECTION_ATTR */
#define JERRY_GLOBAL_HEAP_SECTION __attribute__ ((section (JERRY_HEAP_SECTION_ATTR)))
#endif /* !JERRY_HEAP_SECTION_ATTR */

#ifndef JERRY_SYSTEM_ALLOCATOR
/**
 * Global heap.
 */
jmem_heap_t jerry_global_heap __attribute__ ((aligned (JMEM_ALIGNMENT))) JERRY_GLOBAL_HEAP_SECTION;
#endif /* !JERRY_SYSTEM_ALLOCATOR */

#ifndef CONFIG_ECMA_LCACHE_DISABLE

/**
 * Global hash table.
 */
jerry_hash_table_t jerry_global_hash_table;

#endif /* !CONFIG_ECMA_LCACHE_DISABLE */
#endif /* !JERRY_ENABLE_EXTERNAL_CONTEXT */

jerry_value_t
jcontext_get_backtrace_depth (uint32_t depth)
{
  uint32_t stack_max_depth = JERRY_CONTEXT (stack_max_depth);
  if (depth > stack_max_depth || depth == 0)
  {
    depth = stack_max_depth;
  }

  ecma_value_t frames_array_length_val = ecma_make_uint32_value (depth);
  ecma_value_t frames_array = ecma_op_create_array_object (&frames_array_length_val, 1, true);
  ecma_free_value (frames_array_length_val);
  ecma_object_t *array_p = ecma_get_object_from_value (frames_array);

  vm_frame_ctx_t *ctx_p = JERRY_CONTEXT (vm_top_context_p);
  for (uint32_t idx = 0; idx < depth && ctx_p != NULL; ++idx)
  {
    ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) idx);
    ecma_property_value_t *index_prop_value_p = ecma_create_named_data_property (array_p,
                                                                                 str_idx_p,
                                                                                 ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                                                 NULL);
    ecma_value_t frame = ecma_make_frame (ctx_p->bytecode_header_p);
    ecma_named_data_property_assign_value (array_p, index_prop_value_p, frame);
    ecma_free_value (frame);
    ecma_deref_ecma_string (str_idx_p);

    ctx_p = ctx_p->prev_context_p;
  }

  return ecma_make_object_value (array_p);
}

/**
 * @}
 */
