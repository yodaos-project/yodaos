## Tracing JerryScript heap usage

Adding below arguments when building and running IoT.js will show you the JerryScript memory status.

```text
$ ./tools/build.py --jerry-memstat
$ ./build/bin/iotjs --memstat test.js
Heap stats:
  Heap size = 262136 bytes
  Allocated = 0 bytes
  Waste = 0 bytes
  Peak allocated = 24288 bytes
  Peak waste = 261 bytes
  Skip-ahead ratio = 2.6059
  Average alloc iteration = 1.1284
  Average free iteration = 19.3718

Pools stats:
  Pool chunks: 0
  Peak pool chunks: 735
  Free chunks: 0
  Pool reuse ratio: 0.3459
```

Note that currently only JerryScript heap usage can be shown with memstat option, not IoT.js memory usage. You can use system profiler to trace IoT.js memory usage.

## JerryScript 'external magic string' feature

When parsing and executing JavaScript module, JavaScript strings occupy a huge amount of space in JerryScript heap. To optimize this kind of heap usage, JerryScript has 'external magic string' feature. If you enable snapshot when building, build script will automatically generate `src/iotjs_string_ext.inl.h` file, which includes all of the JavaScript strings used in builtin modules. This file is used by JerryScript to reduce heap usage.

Since same strings will be included only once, you can use this information to get some hints on binary size reduction. Note that only strings with length<32 will be included in this list.

## Placement of JerryScript heap (with an example of STM32F4 CCM Memory)

IoT.js uses two kind of heaps: System heap for normal usage, and separated JerryScript heap for javascript. JerryScript heap is implemented as c array with fixed length decided in static time. Its size can be ~512K.

For some devices, placing this kind of large memory block can be important issue. For example, STM32F4 chips have *Core Coupled Memory* (a.k.a. *ccm* memory), which is usually used as heap or stack. However, since compiler do not have any special knowledge about JerryScript heap, it can be treated as simple array.

You can make your compiler to place the JerryScript heap in specific section by using `--jerry-heap-section` argument when building IoT.js. Your argument will be used with below code in `deps/jerry/jerry-core/jcontext/jcontext.c`

```c
#define JERRY_GLOBAL_HEAP_SECTION __attribute__ ((section (JERRY_HEAP_SECTION_ATTR)))

jmem_heap_t jerry_global_heap __attribute__ ((aligned (JMEM_ALIGNMENT))) JERRY_GLOBAL_HEAP_SECTION;
```

## CPU Profiler
Adding below arguments when building and running IoT.js will enable CPU Profiler.

```shell
$ ./tools/build.py --jerry-cpu-profiler
```

Add Javascript code like docs/api/Profiler.md, Iot.js will generate a Profiler file at runtime, say Profiler-123.

Users can generate collapse stack trace file.
```shell
$ node ./deps/jerry/tools/profiler/j2cl.js Profiler-123  col.txt
```

Then generate flame graph html file.
```shell
$ ./deps/jerry/tools/profiler/flamegraph.pl col.txt > prof.html
```

## Heap Profiler
Adding below arguments when building and running IoT.js will enable Heap Profiler.

```shell
$ ./tools/build.py --jerry-heap-profiler
```

Add JavaScript code like [Profiler docs](..//api/Profiler.md), Iot.js will generate a Jerry Heap Profiler file, say Profiler-123.

Users can generate v8 heap snapshot file.
```shell
$ node deps/jerry/tools/profiler/j2v8snap.js Profiler-123 v8.heapsnapshot
```

Then open in Chrome browser following below site.
[V8 heap profiling docs](https://developer.chrome.com/devtools/docs/heap-profiling)
