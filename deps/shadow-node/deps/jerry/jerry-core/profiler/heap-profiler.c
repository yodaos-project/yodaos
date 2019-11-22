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

#include "heap-profiler.h"
#include "jcontext.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-function-object.h"
#include "ecma-symbol-object.h"
#include "ecma-gc.h"

#ifdef JERRY_HEAP_PROFILER

typedef enum
{
  NODE_TYPE_HIDDEN = 0,
  NODE_TYPE_ARRAY = 1,
  NODE_TYPE_STRING = 2,
  NODE_TYPE_OBJECT = 3,
  NODE_TYPE_CODE = 4,
  NODE_TYPE_CLOSURE = 5,
  NODE_TYPE_REGEXP = 6,
  NODE_TYPE_NUMBER = 7,
  NODE_TYPE_NATIVE = 8,
  NODE_TYPE_STYNTHETIC = 9,
  NODE_TYPE_CONCATENATED_STRING = 10,
  NODE_TYPE_SLICED_STRING = 11,
  NODE_TYPE_SYMBOL = 12,
  NODE_TYPE_BIGINT = 13
} v8_node_type_t;

static void
utf8_string_print (FILE *fp, lit_utf8_byte_t *buffer_p, lit_utf8_size_t sz)
{
  for (uint32_t i = 0; i < sz; i++)
  {
    lit_utf8_byte_t c = buffer_p[i];
    switch (c)
    {
    case '\b':
      fprintf (fp, "\\b");
      break;
    case '\f':
      fprintf (fp, "\\f");
      break;
    case '\n':
      fprintf (fp, "\\n");
      break;
    case '\r':
      fprintf (fp, "\\r");
      break;
    case '\t':
      fprintf (fp, "\\t");
      break;
    case '\"':
      fprintf (fp, "\\\"");
      break;
    case '\\':
      fprintf (fp, "\\\\");
      break;
    default:
      if (c < 128 && c > 31)
      {
        fputc (c, fp);
      }
      else
      {
        fprintf (fp, "u%x,", c);
      }
      break;
    }
  }
}

static void
heapdump_string (FILE *fp, ecma_string_t *string_p)
{
  // print string
  ecma_value_t value = ecma_make_string_value (string_p);
  fprintf (fp, "{\"type\":\"string\",\"id\":%u,\"chars\":\"", value);
  lit_utf8_size_t sz = ecma_string_get_utf8_size (string_p);
  lit_utf8_byte_t buffer_p[sz];
  ecma_string_to_utf8_bytes (string_p, buffer_p, sz);
  utf8_string_print (fp, buffer_p, sz);
  fprintf (fp, "\"},\n");

  // print string node
  fprintf (fp, "{\"type\":\"node\",\"node_type\":%d,\"name\":%u,\"id\":%u,\"size\":%u},\n",
           NODE_TYPE_STRING,
           value,
           value,
           sz + (uint32_t) sizeof(ecma_string_t));
}

static void
heapdump_magic_strings (FILE *fp)
{
  for (lit_magic_string_id_t id = 0;
       id < LIT_NON_INTERNAL_MAGIC_STRING__COUNT;
       id++)
  {
    heapdump_string (fp, ecma_get_magic_string (id));
  }
}

static void
heapdump_literal_strings (FILE *fp)
{
  ecma_lit_storage_item_t *string_list_p = JERRY_CONTEXT (string_list_first_p);

  while (string_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] != JMEM_CP_NULL)
      {
        ecma_string_t *string_p;
        string_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                 string_list_p->values[i]);
        heapdump_string (fp, string_p);
      }
    }

    string_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t,
                                         string_list_p->next_cp);
  }
}

static void
heapdump_number (FILE *fp, ecma_value_t value)
{
  fprintf (fp, "{\"type\":\"node\",\"node_type\":%d,\"name\":%u,\"id\":%u,\"size\":%u},\n",
           NODE_TYPE_NUMBER,
           ecma_make_magic_string_value (LIT_MAGIC_STRING_NUMBER),
           value,
           (uint32_t) sizeof(ecma_value_t));
}

static void
heapdump_literal_numbers (FILE *fp)
{
  ecma_lit_storage_item_t *number_list_p = JERRY_CONTEXT (number_list_first_p);

  while (number_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (number_list_p->values[i] != JMEM_CP_NULL)
      {
        ecma_string_t *value_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
            number_list_p->values[i]);
        heapdump_number (fp, value_p->u.lit_number);
      }
    }

    number_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t,
                                         number_list_p->next_cp);
  }
}

static void
heapdump_object (FILE *fp, ecma_object_t *object_p)
{
  ecma_value_t node_id = ecma_make_object_value (object_p);
  ecma_string_t *node_name = ecma_object_get_name (object_p);
  uint32_t node_self_size = ecma_object_get_size (object_p);
  ecma_object_type_t type = ECMA_OBJECT_TYPE__MAX;
  int node_type;

  if (ecma_is_lexical_environment (object_p))
  {
    node_type = NODE_TYPE_HIDDEN;
  }
  else
  {
    type = ecma_get_object_type (object_p);
    switch (type)
    {
    case ECMA_OBJECT_TYPE_GENERAL:
      node_type = NODE_TYPE_OBJECT;
      break;
    case ECMA_OBJECT_TYPE_CLASS:
      node_type = NODE_TYPE_OBJECT; // ?
      break;
    case ECMA_OBJECT_TYPE_FUNCTION:
      node_type = NODE_TYPE_CLOSURE;
      break;
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
      node_type = NODE_TYPE_CLOSURE;
      break;
    case ECMA_OBJECT_TYPE_ARRAY:
      node_type = NODE_TYPE_ARRAY;
      break;
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
      node_type = NODE_TYPE_CLOSURE;
      break;
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
      node_type = NODE_TYPE_ARRAY;
      break;
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
    case ECMA_OBJECT_TYPE_ARROW_FUNCTION:
      node_type = NODE_TYPE_CLOSURE;
      break;
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
    default:
      JERRY_UNREACHABLE ();
    }
  }

  fprintf (fp, "{\"type\":\"node\",\"node_type\":%d,\"name\":%u,\"id\":%u,\"size\":%u},\n",
           node_type,
           ecma_make_string_value (node_name),
           node_id,
           node_self_size);
  heapdump_string (fp, node_name);

  if (type == ECMA_OBJECT_TYPE_FUNCTION
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
      || type == ECMA_OBJECT_TYPE_ARROW_FUNCTION
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
     )
  {
    if (!ecma_get_object_is_builtin (object_p))
    {
      const ecma_compiled_code_t *bytecode_p;
      if (type == ECMA_OBJECT_TYPE_FUNCTION)
      {
        bytecode_p = ecma_op_function_get_compiled_code ((ecma_extended_object_t*) object_p);
      }
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
      else
      {
        bytecode_p = ecma_op_arrow_function_get_compiled_code ((ecma_arrow_function_t*) object_p);
      }
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
      fprintf (fp, "{\"type\":\"node\",\"node_type\":%d,\"name\":%u,\"id\":%u,\"size\":%u},\n",
               NODE_TYPE_CODE,
               ecma_make_string_value (node_name),
               (ecma_value_t) bytecode_p,
               bytecode_p->size);
      ecma_value_t bytecode_name;
      bytecode_name = ecma_make_magic_string_value(LIT_MAGIC_STRING_BYTECODE);
      fprintf (fp, "{\"type\":\"edge\",\"edge_type\":%u,\"name\":%u,\"from\":%u,\"to\":%u},\n",
          ECMA_REF_TYPE_HIDDEN, bytecode_name, node_id, (ecma_value_t) bytecode_p);
    }

  }
}

static void
heapdump_value (FILE *fp, ecma_value_t value)
{
  if (ecma_is_value_object (value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (value);
    heapdump_object (fp, object_p);
  }
  else if (ecma_is_value_string (value))
  {
    ecma_string_t *string_p = ecma_get_string_from_value (value);
    heapdump_string (fp, string_p);
  }
  else if (ecma_is_value_number (value))
  {
    heapdump_number (fp, value);
  }
  else
  {
    heapdump_number (fp, value); // treat direct value as number
  }
}

static void
heapdump_edge (ecma_object_t *from_p,
               ecma_value_t to,
               ecma_string_t *edge_name_p,
               ecma_ref_type_t edge_type,
               void *args)
{
  FILE *fp = (FILE*) args;
  if(fp)
  {
    ecma_value_t from = ecma_make_object_value (from_p);
    bool deref_edge_name_p = false;
    if (ecma_prop_name_is_symbol (edge_name_p)) {
      ecma_value_t name_value = ecma_make_symbol_value (edge_name_p);
      ecma_value_t desc_value = ecma_get_symbol_descriptive_string (name_value);
      edge_name_p = ecma_get_string_from_value (desc_value);
      deref_edge_name_p = true;
    }
    ecma_value_t name_value = ecma_make_string_value (edge_name_p);
    fprintf (fp, "{\"type\":\"edge\",\"edge_type\":%u,\"name\":%u,\"from\":%u,\"to\":%u},\n",
             edge_type, name_value, from, to);
    heapdump_value (fp, to);
    heapdump_string (fp, edge_name_p);
    if (deref_edge_name_p) {
      ecma_deref_ecma_string (edge_name_p);
    }
  }
}

static void
heapdump_synthetic (FILE *fp)
{
  ecma_value_t to = ecma_make_object_value (JERRY_CONTEXT (ecma_global_lex_env_p));
  ecma_value_t empty_name = ecma_make_string_value (ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY));
  fprintf (fp, "{\"type\":\"node\",\"node_type\":%d,\"name\":%u,\"id\":%u,\"size\":%u},\n",
           NODE_TYPE_STYNTHETIC,
           empty_name,
           0U,
           0U);
  fprintf (fp, "{\"type\":\"edge\",\"edge_type\":%u,\"name\":%u,\"from\":%u,\"to\":%u},\n",
           ECMA_REF_TYPE_SHORTCUT,
           empty_name,
           0U,
           to);
}

void
heap_profiler_take_snapshot (FILE *fp)
{
  ecma_gc_run (JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW);

  fprintf (fp, "{\n");
  fprintf (fp, "\"elements\":[\n");
  heapdump_synthetic (fp);
  heapdump_magic_strings (fp);
  heapdump_literal_strings (fp);
  heapdump_literal_numbers (fp);

  ecma_object_t *obj_iter_p = JERRY_CONTEXT (ecma_gc_objects_p);
  while (obj_iter_p != NULL)
  {
    heapdump_object (fp, obj_iter_p);
    ecma_vist_object_references (obj_iter_p,
                                 heapdump_edge,
                                 (void*) fp);
    obj_iter_p = ECMA_GET_POINTER (ecma_object_t, obj_iter_p->gc_next_cp);
  }

  fprintf (fp, "{\"type\":\"dummy\"}\n");
  fprintf (fp, "]\n"); // elements
  fprintf (fp, "}");
}

#endif /* JERRY_HEAP_PROFILER */
