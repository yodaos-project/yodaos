'use strict';

var EventEmitter = require('events').EventEmitter;
var assert = require('assert');
var common = require('../common');

var bus = new EventEmitter();

function noop() {}

bus.on('foobar', noop);

bus.on('removeListener', common.mustCall((event, fn) => {
  assert.strictEqual(event, 'foobar');
  assert.strictEqual(fn, noop);
}));

bus.removeListener('foobar', noop);

var removeCount = 0;
var emitter = new EventEmitter();

emitter.on('rm', function foo() {
  removeCount++;
  emitter.removeListener('rm', foo);
});
emitter.emit('rm');
assert(removeCount === 1);
emitter.emit('rm');
assert(removeCount === 1); // already removed, do not enter twice

var arrow = () => {
  removeCount++;
  emitter.removeListener('rm', arrow);
};

emitter.on('rm', arrow);
emitter.emit('rm');
assert(removeCount === 2);
emitter.emit('rm');
assert(removeCount === 2);
