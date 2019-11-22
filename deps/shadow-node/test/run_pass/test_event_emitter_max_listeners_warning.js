'use strict';

var common = require('../common');
var events = require('events');
var assert = require('assert');

var e = new events.EventEmitter();
e.setMaxListeners(1);

process.on('warning', common.mustCall((warning) => {
  assert.ok(warning instanceof Error);
  assert.strictEqual(warning.name, 'MaxListenersExceededWarning');
  assert.strictEqual(warning.emitter, e);
  assert.strictEqual(warning.count, 2);
  assert.strictEqual(warning.type, 'event-type');
}));

function noop() {}

e.on('event-type', noop);
e.on('event-type', noop);
e.on('event-type', noop);
e.on('event-type', noop);
