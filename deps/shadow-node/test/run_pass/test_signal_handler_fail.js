'use strict';

var assert = require('assert');
function shouldNotHandleSignal(type) {
  assert.throws(() => {
    process.on(type, () => false);
  }, {
    name: 'Error',
    message: 'uv_signal_start EINVAL(invalid argument)'
  }, `should throw uv_signal_start(${type}) invalid args`);
}

shouldNotHandleSignal('SIGKILL');
shouldNotHandleSignal('SIGSTOP');
