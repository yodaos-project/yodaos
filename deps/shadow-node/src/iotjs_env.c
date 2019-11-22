/* Copyright 2015-present Samsung Electronics Co., Ltd. and other contributors
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


#include "iotjs_def.h"
#include "iotjs_env.h"
#include "iotjs_handlewrap.h"

#include <string.h>


static iotjs_environment_t current_env;
static bool initialized = false;


/**
 * Constructor/Destructor on private section.
 * To prevent create an instance of iotjs_environment_t.
 * The only way to get an instance of environment is iotjs_environment_get()
 */


static void iotjs_environment_initialize(iotjs_environment_t* env);
static void iotjs_environment_destroy(iotjs_environment_t* env);


/**
 * Get the singleton instance of iotjs_environment_t.
 */
iotjs_environment_t* iotjs_environment_get() {
  if (!initialized) {
    iotjs_environment_initialize(&current_env);
    initialized = true;
  }
  return &current_env;
}


/**
 * Release the singleton instance of iotjs_environment_t, and debugger config.
 */
void iotjs_environment_release() {
  if (initialized) {
    if (iotjs_environment_config(iotjs_environment_get())->debugger != NULL) {
      iotjs_buffer_release(
          (char*)iotjs_environment_config(iotjs_environment_get())->debugger);
    }
    iotjs_environment_destroy(&current_env);
    initialized = false;
  }
}


/**
 * Initialize an instance of iotjs_environment_t.
 */
static void iotjs_environment_initialize(iotjs_environment_t* env) {
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_environment_t, env);

  _this->argc = 0;
  _this->argv = NULL;
  _this->loop = NULL;
  _this->state = kInitializing;
  _this->handlewrap_queue = list_new();
  _this->config.memstat = false;
  _this->config.show_opcode = false;
  _this->config.debugger = NULL;
}


/**
 * Destroy an instance of iotjs_environment_t.
 */
static void iotjs_environment_destroy(iotjs_environment_t* env) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_environment_t, env);
  if (_this->handlewrap_queue != NULL)
    list_destroy(_this->handlewrap_queue);

  if (_this->argv) {
    // release command line argument strings.
    // _argv[0] and _argv[1] refer addresses in static memory space.
    // Others refer addresses in heap space that is need to be deallocated.
    for (uint32_t i = 2; i < _this->argc; ++i) {
      iotjs_buffer_release(_this->argv[i]);
    }
    iotjs_buffer_release((char*)_this->argv);
  }

  if (_this->immediate_check_handle != NULL) {
    IOTJS_RELEASE(_this->immediate_check_handle);
  }
  if (_this->immediate_idle_handle != NULL) {
    IOTJS_RELEASE(_this->immediate_idle_handle);
  }
}

void iotjs_environment_print_help() {
  fprintf(
      stderr,
      "Usage: iotjs [options] [script | script.js] [arguments]\n\n"
      "Options:\n"
      "  -v, --version              print version\n"
      "  -h, --help                 print help\n"
      "  --loadstat                 print the load statistics\n"
      "  --memstat                  print the memory statistics\n"
      "  --show-opcodes             print the opcodes of running script\n"
      "  --start-debug-server       activate the debugger server\n\n"
      "Environment variables:\n"
      "NODE_DISABLE_COLORS          set to 1 to disable colors in the REPL\n"
      "NODE_PATH                    ':'-separated list of directories\n"
      "                             prefixed to the module search path\n"
      "\nDocumentation can be found at "
      "https://github.com/Rokid/ShadowNode/tree/master/docs\n");
}

void iotjs_environment_print_version() {
  fprintf(stdout, "v" IOTJS_VERSION "\n");
}

/**
 * Parse command line arguments
 */
bool iotjs_environment_parse_command_line_arguments(iotjs_environment_t* env,
                                                    uint32_t argc,
                                                    char** argv) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);

  // set uv args
  uv_setup_args((int)argc, argv);

  // Parse IoT.js command line arguments.
  uint32_t i = 1;
  uint8_t port_arg_len = strlen("--jerry-debugger-port=");
  while (i < argc && argv[i][0] == '-') {
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      iotjs_environment_print_help();
      return false;
    } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
      iotjs_environment_print_version();
      return false;
    } else if (!strcmp(argv[i], "--loadstat")) {
      _this->config.loadstat = true;
    } else if (!strcmp(argv[i], "--memstat")) {
      _this->config.memstat = true;
    } else if (!strcmp(argv[i], "--show-opcodes")) {
      _this->config.show_opcode = true;
    } else if (!strcmp(argv[i], "--start-debug-server")) {
      _this->config.debugger =
          (DebuggerConfig*)iotjs_buffer_allocate(sizeof(DebuggerConfig));
      _this->config.debugger->port = 5001;
      _this->config.debugger->wait_source = false;
      _this->config.debugger->context_reset = false;
    } else if (!strncmp(argv[i], "--jerry-debugger-port=", port_arg_len) &&
               _this->config.debugger) {
      size_t port_length = sizeof(strlen(argv[i] - port_arg_len - 1));
      char port[port_length];
      memcpy(&port, argv[i] + port_arg_len, port_length);
      sscanf(port, "%d", &(_this->config.debugger->port));
    } else if (!strcmp(argv[i], "--debugger-wait-source") &&
               _this->config.debugger) {
      _this->config.debugger->wait_source = true;
    } else {
      fprintf(stderr, "unknown command line option: %s\n", argv[i]);
      return false;
    }
    ++i;
  }

  // There must be at least one argument after processing the IoT.js args,
  // except when sources are sent over by the debugger client.
  if ((argc - i) < 1 && (_this->config.debugger == NULL ||
                         !_this->config.debugger->wait_source)) {
    iotjs_environment_print_help();
    return false;
  }

  // If waiting for source is enabled, there is no need to handle
  // commandline args.
  // Remaining arguments are for application.
  if (_this->config.debugger == NULL || !_this->config.debugger->wait_source) {
    _this->argc = 2;
    size_t buffer_size = ((size_t)(_this->argc + argc - i)) * sizeof(char*);
    _this->argv = (char**)iotjs_buffer_allocate(buffer_size);
    _this->argv[0] = argv[0];
    _this->argv[1] = argv[i++];
    while (i < argc) {
      _this->argv[_this->argc] = iotjs_buffer_allocate(strlen(argv[i]) + 1);
      strcpy(_this->argv[_this->argc], argv[i]);
      _this->argc++;
      i++;
    }
  }

  return true;
}

uint32_t iotjs_environment_argc(const iotjs_environment_t* env) {
  const IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  return _this->argc;
}


const char* iotjs_environment_argv(const iotjs_environment_t* env,
                                   uint32_t idx) {
  const IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  return _this->argv[idx];
}


uv_loop_t* iotjs_environment_loop(const iotjs_environment_t* env) {
  const IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  return _this->loop;
}


void iotjs_environment_set_loop(iotjs_environment_t* env, uv_loop_t* loop) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  _this->loop = loop;
}


const Config* iotjs_environment_config(const iotjs_environment_t* env) {
  const IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  return &_this->config;
}


void iotjs_environment_create_handlewrap(void* handlewrap) {
  const iotjs_environment_t* env = iotjs_environment_get();
  const IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  list_rpush(_this->handlewrap_queue, list_node_new(handlewrap));
}


void iotjs_environment_remove_handlewrap(void* handlewrap) {
  const iotjs_environment_t* env = iotjs_environment_get();
  const IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  list_node_t* node = list_find(_this->handlewrap_queue, handlewrap);
  if (node != NULL)
    list_remove(_this->handlewrap_queue, node);
}


void iotjs_environment_cleanup_handlewrap() {
  const iotjs_environment_t* env = iotjs_environment_get();
  const IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);

  list_node_t* node;
  list_iterator_t* it = list_iterator_new(_this->handlewrap_queue, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    iotjs_handlewrap_t* handlewrap = (iotjs_handlewrap_t*)(node->val);
    if (handlewrap != NULL) {
      iotjs_handlewrap_validate(handlewrap);
      iotjs_handlewrap_close(handlewrap, NULL);
    }
  }
  list_iterator_destroy(it);
}


void iotjs_environment_go_state_running_main(iotjs_environment_t* env) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);

  IOTJS_ASSERT(_this->state == kInitializing);
  _this->state = kRunningMain;
}


void iotjs_environment_go_state_running_loop(iotjs_environment_t* env) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);

  IOTJS_ASSERT(_this->state == kRunningMain);
  _this->state = kRunningLoop;
}


void iotjs_environment_go_state_exiting(iotjs_environment_t* env) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  IOTJS_ASSERT(_this->state < kExiting);
  _this->state = kExiting;
}

bool iotjs_environment_is_exiting(iotjs_environment_t* env) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_environment_t, env);
  return _this->state == kExiting;
}
