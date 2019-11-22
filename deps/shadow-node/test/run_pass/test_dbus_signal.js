'use strict';

var assert = require('assert');
var dbus = require('dbus');

var SERVICE_NAME = 'org.myservice';
var OBJECT_PATH = '/org/myobject';
var INTERFACE_1 = 'test.dbus.myservice.Interface1';

var myservice = dbus.registerService('session', SERVICE_NAME);
var myobject = myservice.createObject(OBJECT_PATH);
var myiface = myobject.createInterface(INTERFACE_1);

myiface.addSignal('beep', {
  types: [ dbus.Define(String) ],
});
myiface.addSignal('mua', {
  types: [ dbus.Define(String), dbus.Define(String) ],
});
myiface.update();

var bus = dbus.getBus();
var plan = 2;

bus.getUniqueServiceName(SERVICE_NAME, (err, id) => {
  bus.addSignalFilter(id, OBJECT_PATH, INTERFACE_1);
  bus.on(`${id}:${OBJECT_PATH}:${INTERFACE_1}`, (value) => {
    if (value.name === 'beep') {
      plan -= 1;
      assert.strictEqual(value.args.length, 1);
      assert.strictEqual(value.args[0], 'foobar');
    } else if (value.name === 'mua') {
      plan -= 1;
      assert.strictEqual(value.args.length, 2);
      assert.strictEqual(value.args[0], 'mew~');
      assert.strictEqual(value.args[1], 'mew~');
    }
  });
  myiface.emit('beep', ['foobar']);
  myiface.emit('mua', ['mew~', 'mew~']);
});

setTimeout(() => {
  assert.strictEqual(plan, 0);
  bus.destroy();
}, 500);
