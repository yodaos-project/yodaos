# Zlib

> Stability: 2 - Stable

The `zlib` module provides compression functionality implemented using Gzip and
Deflate/Inflate. It can be accessed using:

```js
var zlib = require('zlib');
```

Compressing or decompressing a stream (such as a file) can be accomplished by
piping the source stream data through a `zlib` stream into a destination stream:

```js
var gzip = zlib.createGzip();
var fs = require('fs');
var inp = fs.createReadStream('input.txt');
var out = fs.createWriteStream('input.txt.gz');

inp.pipe(gzip).pipe(out);
```

## Threadpool Usage

Note that all zlib APIs except those that are explicitly synchronous use libuv's
threadpool, which can have surprising and negative performance implications for
some applications, see the [`UV_THREADPOOL_SIZE`][] documentation for more
information.

## Memory Usage Tuning

<!--type=misc-->

From `zlib/zconf.h`, modified to node.js's usage:

The memory requirements for deflate are (in bytes):

<!-- eslint-disable semi -->
```js
(1 << (windowBits + 2)) + (1 << (memLevel + 9))
```

That is: 128K for windowBits=15  +  128K for memLevel = 8
(default values) plus a few kilobytes for small objects.

For example, to reduce the default memory requirements from 256K to 128K, the
options should be set to:

```js
var options = { windowBits: 14, memLevel: 7 };
```

This will, however, generally degrade compression.

The memory requirements for inflate are (in bytes) `1 << windowBits`.
That is, 32K for windowBits=15 (default value) plus a few kilobytes
for small objects.

This is in addition to a single internal output slab buffer of size
`chunkSize`, which defaults to 16K.

The speed of `zlib` compression is affected most dramatically by the
`level` setting.  A higher level will result in better compression, but
will take longer to complete.  A lower level will result in less
compression, but will be much faster.

In general, greater memory usage options will mean that Node.js has to make
fewer calls to `zlib` because it will be able to process more data on
each `write` operation.  So, this is another factor that affects the
speed, at the cost of memory usage.

## Class Options

<!--type=misc-->

Each class takes an `options` object.  All options are optional.

Note that some options are only relevant when compressing, and are
ignored by the decompression classes.

* `flush` {integer} (default: `zlib.constants.Z_NO_FLUSH`)
* `finishFlush` {integer} (default: `zlib.constants.Z_FINISH`)
* `chunkSize` {integer} (default: 16\*1024)
* `windowBits` {integer}
* `level` {integer} (compression only)
* `memLevel` {integer} (compression only)
* `strategy` {integer} (compression only)
* `dictionary` {Buffer|TypedArray|DataView|ArrayBuffer} (deflate/inflate only,
  empty dictionary by default)
* `info` {boolean} (If `true`, returns an object with `buffer` and `engine`)

See the description of `deflateInit2` and `inflateInit2` at
<https://zlib.net/manual.html#Advanced> for more information on these.

## Class: zlib.Deflate

Compress data using deflate.

## Class: zlib.DeflateRaw

Compress data using deflate, and do not append a `zlib` header.

## Class: zlib.Gunzip

Decompress a gzip stream.

## Class: zlib.Gzip

Compress data using gzip.

## Class: zlib.Inflate

Decompress a deflate stream.

## Class: zlib.InflateRaw

Decompress a raw deflate stream.

## Class: zlib.Unzip

Decompress either a Gzip- or Deflate-compressed stream by auto-detecting
the header.

## Class: zlib.Zlib

Not exported by the `zlib` module. It is documented here because it is the base
class of the compressor/decompressor classes.

### zlib.bytesRead

* {number}

The `zlib.bytesRead` property specifies the number of bytes read by the engine
before the bytes are processed (compressed or decompressed, as appropriate for
the derived class).

### zlib.close([callback])

Close the underlying handle.

### zlib.flush([kind], callback)

`kind` defaults to `zlib.constants.Z_FULL_FLUSH`.

Flush pending data. Don't call this frivolously, premature flushes negatively
impact the effectiveness of the compression algorithm.

Calling this only flushes data from the internal `zlib` state, and does not
perform flushing of any kind on the streams level. Rather, it behaves like a
normal call to `.write()`, i.e. it will be queued up behind other pending
writes and will only produce output when data is being read from the stream.

### zlib.params(level, strategy, callback)

Dynamically update the compression level and compression strategy.
Only applicable to deflate algorithm.

### zlib.reset()

Reset the compressor/decompressor to factory defaults. Only applicable to
the inflate and deflate algorithms.

## zlib.constants

Provides an object enumerating Zlib-related constants.

## zlib.createDeflate([options])

Creates and returns a new [Deflate][] object with the given [options][].

## zlib.createDeflateRaw([options])

Creates and returns a new [DeflateRaw][] object with the given [options][].

*Note*: An upgrade of zlib from 1.2.8 to 1.2.11 changed behavior when windowBits
is set to 8 for raw deflate streams. zlib would automatically set windowBits
to 9 if was initially set to 8. Newer versions of zlib will throw an exception,
so Node.js restored the original behavior of upgrading a value of 8 to 9,
since passing `windowBits = 9` to zlib actually results in a compressed stream
that effectively uses an 8-bit window only.

## zlib.createGunzip([options])

Creates and returns a new [Gunzip][] object with the given [options][].

## zlib.createGzip([options])

Creates and returns a new [Gzip][] object with the given [options][].

## zlib.createInflate([options])

Creates and returns a new [Inflate][] object with the given [options][].

## zlib.createInflateRaw([options])

Creates and returns a new [InflateRaw][] object with the given [options][].

## zlib.createUnzip([options])

Creates and returns a new [Unzip][] object with the given [options][].
