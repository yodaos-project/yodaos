'use strict';

var assert = require('assert');
var dbus = require('dbus');

var myservice = dbus.registerService('session', 'org.myservice');
var myobject = myservice.createObject('/org/myobject');
var myiface = myobject.createInterface('test.dbus.myservice.Interface1');

myiface.addMethod('test', {
  out: [dbus.Define(String)] }, (cb) => {
  cb(null, 'simple call');
});
myiface.addMethod('testWithArgs', {
  in: [dbus.Define(String)],
  out: [dbus.Define(String)] }, (num, cb) => {
  cb(null, num + '!!!');
});
myiface.addMethod('testNumber', {
  in: [],
  out: [dbus.Define(Number)] }, (cb) => {
  cb(null, 9999);
});
myiface.addMethod('testDouble', {
  in: [],
  out: [dbus.Define(Number)] }, (cb) => {
  cb(null, 0.1);
});
myiface.update();

var bus = dbus.getBus();
var plan = 0;
// start to call
bus.callMethod(
  'org.myservice',
  '/org/myobject',
  'test.dbus.myservice.Interface1', 'test', '', [], (err, res) => {
    assert.strictEqual(err, null);
    assert.strictEqual(res, 'simple call');
    plan += 1;
  });

bus.callMethod(
  'org.myservice',
  '/org/myobject',
  'test.dbus.myservice.Interface1',
  'testWithArgs', 's', ['foobar'], (err, res) => {
    assert.strictEqual(err, null);
    assert.strictEqual(res, 'foobar!!!');
    plan += 1;
  });

bus.callMethod(
  'org.myservice',
  '/org/myobject',
  'test.dbus.myservice.Interface1', 'testNumber', 's', [], (err, res) => {
    assert.strictEqual(err, null);
    assert.strictEqual(res, 9999);
    plan += 1;
  });

bus.callMethod(
  'org.myservice',
  '/org/myobject',
  'test.dbus.myservice.Interface1', 'testDouble', 's', [], (err, res) => {
    assert.strictEqual(err, null);
    assert.strictEqual(res, 0.1);
    plan += 1;
  });

setTimeout(function() {
  assert.strictEqual(plan, 4);
  bus.destroy();
}, 500);
