'use strict';
var assert = require('assert');
// json length is close to 19KB
var data = require('./test_process_send.json');
var dataStr = JSON.stringify(data);

function equalData(msg) {
  if (typeof msg === 'object') {
    console.log('stringify msg');
    msg = JSON.stringify(msg);
    // assert.strictEqual(msg.length, JSON.stringify(data).length);
  } else {
    console.log('string msg');
  }
  assert.strictEqual(msg.length, dataStr.length);
}

var obj = null;
if (process.send) {
  obj = process;
} else {
  var fork = require('child_process').fork;
  obj = fork(module.filename, [], {});
}
obj.on('message', equalData);

obj.send(data);
obj.send(dataStr);

setTimeout(() => {}, 1000);
