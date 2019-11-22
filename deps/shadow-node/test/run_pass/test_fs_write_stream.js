'use strict';

var fs = require('fs');
var path = require('path');
var assert = require('assert');
var common = require('../common');

var dstFilePath = path.join(__dirname, '../resources/test2.txt');
var originalContents = fs.readFileSync(dstFilePath);
var buff1 = Buffer.from('IoT');
var buff2 = Buffer.from('.js');

function noop() {}

var stream = fs.createWriteStream(dstFilePath);
stream.write(buff1, noop);
stream.write(buff2, noop);
stream.on('finish', common.mustCall(function onfinish() {
  var outputs = fs.readFileSync(dstFilePath, 'utf8');
  assert.strictEqual(outputs, 'IoT.js');
  fs.writeFileSync(dstFilePath, originalContents);
}));
stream.end();
