'use strict';

var assert = require('assert');
var dbus = require('dbus');

var myservice = dbus.registerService('session', 'org.myservice');
var myobject = myservice.createObject('/org/myobject');
var myiface = myobject.createInterface('test.dbus.myservice.Interface1');

myiface.addMethod('test', {
  out: [dbus.Define(String)] }, (cb) => {
  cb(null, 'result');
});
myiface.addMethod('test2', {
  in: [dbus.Define(String)], out: [dbus.Define(String)] }, (arg1, cb) => {
  setTimeout(() => {
    cb(null, arg1 + '>!');
  }, 2000);
});
myiface.update();

var bus = dbus.getBus();
bus.getInterface(
  'org.myservice',
  '/org/myobject',
  'test.dbus.myservice.Interface1', (err, iface) => {
    assert.strictEqual(typeof iface.test, 'function');
    iface.test((err, result) => {
      assert.strictEqual(result, 'result');
      iface.test2('foobar', (err, result) => {
        assert.strictEqual(result, 'foobar>!');
        // quit
        bus.destroy();
      });
    });
  });
