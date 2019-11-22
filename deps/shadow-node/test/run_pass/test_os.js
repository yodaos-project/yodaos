'use strict';

var assert = require('assert');
var os = require('os');
var interfaces = os.networkInterfaces();
var actual = {};

switch (os.platform()) {
  case 'linux': {
    actual = interfaces.lo.filter(function(e) {
      return e.address === '127.0.0.1' &&
             e.netmask === '255.0.0.0' &&
             e.family === 'IPv4';
    });
    break;
  }
  case 'darwin': {
    actual = interfaces.lo0.filter(function(e) {
      return e.address === '127.0.0.1' &&
             e.netmask === '255.0.0.0' &&
             e.family === 'IPv4';
    });
    break;
  }
}

var expected = [
  {
    address: '127.0.0.1',
    netmask: '255.0.0.0',
    family: 'IPv4',
    broadcast: '127.0.0.1',
    mac: '00:00:00:00:00:00'
  }
];

assert.deepStrictEqual(actual, expected);
