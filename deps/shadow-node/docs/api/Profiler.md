# Profiler

The `profiler` module provides memory and CPU profiler methods. It can be accessed using:

```js
var profiler = require('profiler');
```

### profiler.startProfiling()
* Returns {null}

The `profiler.startProfiling(path)` method start a CPU profiler, which should be explicitly stopped by profiler.stopProfiling

The profiling data will be stored into a file ${process.cwd()}/Profile-${Date.now()}.

### profiler.startProfiling(path)
* `path` {String} profiling data file path
* Returns {null}

The `profiler.startProfiling(path)` method start a CPU profiler, which should be explicitly stopped by profiler.stopProfiling

### profiler.startProfiling(duration)
* `duration` {Number}
* Returns {null}

The `profiler.startProfiling(duration)` method start a CPU profiler, which will stop after `duration` milliseconds.

The profiling data will be stored into a file ${process.cwd()}/Profile-${Date.now()}.

### profiler.startProfiling(path, duration)
* `path` {String} profiling data file path
* `duration` {Number}
* Returns {null}

The `profiler.startProfiling(path, duration)` method start a CPU profiler, which will stop after `duration` milliseconds.

### profiler.takeSnapshot([path])
* `path` {String} profiling data file path
* Returns {null}

The `profiler.takeSnapshot()` methd take a Heap profiler file which path is `path`.
If path is not provided, it will be set to `${process.cwd()}/Profile-${Date.now()}`


**Example**

```js
var profiler = require('profiler');
profiler.startProfiling();
balabala...
profiler.stopProfiling();
```

```js
var profiler = require('profiler');
profiler.startProfiling('/data/profile-123.txt');
balabala...
profiler.stopProfiling();
```

```js
var profiler = require('profiler');
profiler.startProfiling(1000);
balabala...
```

```js
var profiler = require('profiler');
profiler.startProfiling('/data/profile-123.txt', 1000);
balabala...
```

```js
var profiler = require('profiler');
profiler.takeSnapshot();
```

```js
var profiler = require('profiler');
profiler.takeSnapshot('/data/profile-123.txt');
