'use strict';

var common = require('../../common');
var assert = require('assert');
var async_hooks = require('async_hooks');
var binding = require(`./build/Release/binding.node`);
var makeCallback = binding.makeCallback;

// Check async hooks integration using async context.
var hook_result = {
  id: null,
  init_called: false,
  before_called: false,
  after_called: false,
  destroy_called: false,
};
var test_hook = async_hooks.createHook({
  init: (id, type) => {
    if (type === 'test') {
      hook_result.id = id;
      hook_result.init_called = true;
    }
  },
  before: (id) => {
    if (id === hook_result.id) hook_result.before_called = true;
  },
  after: (id) => {
    if (id === hook_result.id) hook_result.after_called = true;
  },
  destroy: (id) => {
    if (id === hook_result.id) hook_result.destroy_called = true;
  },
});

test_hook.enable();
makeCallback(process, function() {});

assert.strictEqual(hook_result.init_called, true);
assert.strictEqual(hook_result.before_called, true);
assert.strictEqual(hook_result.after_called, true);
setImmediate(() => {
  assert.strictEqual(hook_result.destroy_called, true);
  test_hook.disable();
});
