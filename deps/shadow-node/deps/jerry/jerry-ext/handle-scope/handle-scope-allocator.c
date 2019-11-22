/*
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

#include <stdlib.h>
#include "handle-scope-internal.h"

static jerryx_handle_scope_t kJerryXHandleScopeRoot = {
  .handle_count = 0,
  .handle_ptr = NULL,
};
static jerryx_handle_scope_t *kJerryXHandleScopeCurrent = &kJerryXHandleScopeRoot;
static jerryx_handle_scope_pool_t kJerryXHandleScopePool = {
  .count = 0,
  .start = NULL,
};

#define kJerryXHandleScopePoolPrelistLast \
  kJerryXHandleScopePool.prelist + JERRYX_SCOPE_PRELIST_SIZE - 1

#define JerryXHandleScopePrelistIdx(scope) (scope - kJerryXHandleScopePool.prelist)


/**
 * Get current handle scope top of stack.
 */
inline
jerryx_handle_scope_t *
jerryx_handle_scope_get_current (void)
{
  return kJerryXHandleScopeCurrent;
}


/**
 * Get root handle scope.
 */
inline
jerryx_handle_scope_t *
jerryx_handle_scope_get_root (void)
{
  return &kJerryXHandleScopeRoot;
}


/**
 * Determines if given handle scope is located in pre-allocated list.
 */
static
inline
bool
jerryx_handle_scope_is_in_prelist (jerryx_handle_scope_t *scope)
{
  return (kJerryXHandleScopePool.prelist <= scope) &&
    (scope <= (kJerryXHandleScopePool.prelist + JERRYX_SCOPE_PRELIST_SIZE - 1));
}


/**
 * Get the parent of given handle scope.
 * If given handle scope is in prelist, the parent must be in prelist too;
 * if given is the first item of heap chain list, the parent must be the last one of prelist;
 * the parent must be in chain list otherwise.
 */
jerryx_handle_scope_t *
jerryx_handle_scope_get_parent (jerryx_handle_scope_t *scope)
{
  if (scope == &kJerryXHandleScopeRoot)
  {
    return NULL;
  }
  if (!jerryx_handle_scope_is_in_prelist (scope))
  {
    jerryx_handle_scope_dynamic_t *dy_scope = (jerryx_handle_scope_dynamic_t *) scope;
    if (dy_scope == kJerryXHandleScopePool.start)
    {
      return kJerryXHandleScopePoolPrelistLast;
    }
    jerryx_handle_scope_dynamic_t *parent = dy_scope->parent;
    return (jerryx_handle_scope_t *) parent;
  }
  if (scope == kJerryXHandleScopePool.prelist)
  {
    return &kJerryXHandleScopeRoot;
  }
  return kJerryXHandleScopePool.prelist + JerryXHandleScopePrelistIdx (scope) - 1;
}


/**
 * Get the child of given handle scope.
 * If the given handle scope is in heap chain list, its child must be in heap chain list too;
 * if the given handle scope is the last one of prelist, its child must be the first item of chain list;
 * the children are in prelist otherwise.
 */
jerryx_handle_scope_t *
jerryx_handle_scope_get_child (jerryx_handle_scope_t *scope)
{
  if (scope == &kJerryXHandleScopeRoot)
  {
    if (kJerryXHandleScopePool.count > 0) {
      return kJerryXHandleScopePool.prelist;
    }
    return NULL;
  }
  if (!jerryx_handle_scope_is_in_prelist (scope))
  {
    jerryx_handle_scope_dynamic_t *child = ((jerryx_handle_scope_dynamic_t *) scope)->child;
    return (jerryx_handle_scope_t *) child;
  }
  if (scope == kJerryXHandleScopePoolPrelistLast)
  {
    return (jerryx_handle_scope_t *) kJerryXHandleScopePool.start;
  }
  long idx = JerryXHandleScopePrelistIdx (scope);
  if (idx < 0)
  {
    return NULL;
  }
  if ((unsigned long) idx >= kJerryXHandleScopePool.count - 1)
  {
    return NULL;
  }
  return kJerryXHandleScopePool.prelist + idx + 1;
}


/**
 * Claims a handle scope either from prelist or allocating a new memory block,
 * and increment pool's scope count by 1, and set current scope to the newly claimed one.
 * If there are still available spaces in prelist, claims a block in prelist;
 * otherwise allocates a new memory block from heap and sets its fields to default values,
 * and link it to previously dynamically allocated scope, or link it to pool's start pointer.
 *
 * @returns the newly claimed handle scope pointer.
 */
jerryx_handle_scope_t *
jerryx_handle_scope_alloc (void)
{
  jerryx_handle_scope_t *scope;
  if (kJerryXHandleScopePool.count < JERRYX_SCOPE_PRELIST_SIZE)
  {
    scope = kJerryXHandleScopePool.prelist + kJerryXHandleScopePool.count;
    goto deferred;
  }

  do {
    jerryx_handle_scope_dynamic_t *dy_scope = malloc (sizeof(jerryx_handle_scope_dynamic_t));
    JERRYX_HANDLE_SCOPE_ASSERT(dy_scope != NULL);
    dy_scope->child = NULL;

    if (kJerryXHandleScopePool.count != JERRYX_SCOPE_PRELIST_SIZE)
    {
      jerryx_handle_scope_dynamic_t *dy_current = (jerryx_handle_scope_dynamic_t *) kJerryXHandleScopeCurrent;
      dy_scope->parent = dy_current;
      dy_current->child = dy_scope;
    }
    else
    {
      kJerryXHandleScopePool.start = dy_scope;
      dy_scope->parent = NULL;
    }

    scope = (jerryx_handle_scope_t *) dy_scope;
  } while (0);

deferred:
  scope->handle_count = 0;
  scope->escaped = false;
  scope->handle_ptr = NULL;

  kJerryXHandleScopeCurrent = scope;
  kJerryXHandleScopePool.count += 1;
  return (jerryx_handle_scope_t *) scope;
}


/**
 * Deannounce a previously claimed handle scope, return it to pool
 * or free the allocated memory block.
 */
void
jerryx_handle_scope_free (jerryx_handle_scope_t *scope)
{
  if (scope == &kJerryXHandleScopeRoot)
  {
    return;
  }

  kJerryXHandleScopePool.count -= 1;
  if (scope == kJerryXHandleScopeCurrent) {
    kJerryXHandleScopeCurrent = jerryx_handle_scope_get_parent(scope);
  }

  if (!jerryx_handle_scope_is_in_prelist (scope))
  {
    jerryx_handle_scope_dynamic_t *dy_scope = (jerryx_handle_scope_dynamic_t *) scope;
    if (dy_scope == kJerryXHandleScopePool.start)
    {
      kJerryXHandleScopePool.start = dy_scope->child;
    }
    else if (dy_scope->parent != NULL)
    {
      dy_scope->parent->child = dy_scope->child;
    }
    free (dy_scope);
    return;
  }
  /**
   * Nothing to do with scopes in prelist
   */
}
