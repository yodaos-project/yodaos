# N-API

Most N-API api in ShadowNode are aligned with definition of Node.js version 10.8. Yet there are some that may not implemented nor not supported. Documents of supported API can be found in [Node.js N-API](https://nodejs.org/docs/latest-v10.x/api/n-api.html) document. Following is a list of api that can not be used in ShadowNode.

## How to build

Use [node-gyp](https://github.com/nodejs/node-gyp) whenever possible, since N-API is designed to be ABI stable.

## API not supported in ShadowNode

#### Working with JavaScript Values

##### Object Creation Functions
- napi_create_external_buffer
- napi_create_dataview

##### Functions to convert from C types to N-API
- napi_create_bigint_int64
- napi_create_bigint_uint64
- napi_create_bigint_words
- napi_create_string_latin1
- napi_create_string_utf16

##### Functions to convert from N-API to C types
- napi_get_dataview_info
- napi_get_value_bigint_int64
- napi_get_value_bigint_uint64
- napi_get_value_bigint_words
- napi_get_value_string_latin1
- napi_get_value_string_utf16

#### Working with JavaScript Values - Abstract Operations
- napi_is_dataview

#### Custom Asynchronous Operations
Though N-API functions for creating/destroying async contexts are available, they do not work as expected as `async_hooks` has not been implemented.

#### Memory Management
- napi_adjust_external_memory

#### Promises
- napi_create_promise
- napi_resolve_deferred
- napi_reject_deferred
- napi_is_promise

#### Script execution
- napi_run_script

#### Experimental functions
- napi_add_finalizer
