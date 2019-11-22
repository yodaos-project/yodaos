'use strict';

var common = require('../common');
var assert = require('assert');
var signals = require('constants').os.signals;

function testSignal(type, platform) {
  if (arguments.length === 2 &&
    platform !== process.platform) {
    console.log(`skiped, test ${type} only on ` +
                `${platform}, but ${process.platform}`);
    return;
  }
  process.once(type, common.mustCall(function(signal) {
    console.log(signal);
    assert.strictEqual(signal, type);
  }));
  process.kill(process.pid, signals[type]);
}

testSignal('SIGHUP');
testSignal('SIGINT');
testSignal('SIGQUIT');
testSignal('SIGILL');
testSignal('SIGTRAP');
testSignal('SIGABRT');
testSignal('SIGBUS');
testSignal('SIGFPE');
testSignal('SIGUSR1');
testSignal('SIGSEGV');
testSignal('SIGUSR2');
testSignal('SIGPIPE');
testSignal('SIGALRM');
testSignal('SIGTERM');
testSignal('SIGCHLD');
testSignal('SIGSTKFLT', 'linux');
