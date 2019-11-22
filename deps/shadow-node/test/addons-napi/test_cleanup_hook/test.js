'use strict';
var common = require('../../common');
var assert = require('assert');
var child_process = require('child_process');

if (process.argv[2] === 'child') {
  require(`./build/Release/binding.node`);
} else {
  var { stdout } =
    child_process.spawnSync(process.execPath, [__filename, 'child']);
  assert.strictEqual(stdout.toString().trim(), 'cleanup(42)');
}
