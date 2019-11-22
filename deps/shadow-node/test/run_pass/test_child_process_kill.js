'use strict';

var assert = require('assert');
var path = require('path');
var fork = require('child_process').fork;
var grep = fork(
  path.join(__dirname, 'test_child_process_kill2.js'),
  {
    execArgv: [],
  }
);

var closed;
var exited;

grep.on('close', function(code, signal) {
  assert.strictEqual(signal, 'SIGTERM');
  closed = true;
});

grep.on('exit', function(code, signal) {
  assert.strictEqual(signal, 'SIGTERM');
  exited = true;
});

// Send SIGHUP to process
var killed = grep.kill();
assert.strictEqual(killed, true);
assert.strictEqual(killed, grep.killed);

process.on('exit', function() {
  console.log('exit');
  assert.strictEqual(closed, true);
  assert.strictEqual(exited, true);
});
