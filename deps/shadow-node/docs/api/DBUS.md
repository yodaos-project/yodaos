# DBus

DBus is the module for IPC.

**Example**

```js
var dbus = require('dbus').getBus('session');

dbus.getInterface(
  'org.an.service-name', 
  '/path/to/object', 
  'ifaceName', 
  function(err, iface) {
    iface.someMethod();
  }
);
```

## getInterface(serviceName, objectPath, interfaceName, cb)

* `serviceName` {String} the service name.
* `objectPath` {String} the object path.
* `interfaceName` {String} the interface name.
* `cb` {Function} the callback when the interface is loaded.

Gets the interface with specified service, object and name.

## registerService(channel, name)

* `channel` {String} the channel to register, "session" or "system".
* `name` {String} the service name.

register the servie with specified name, it returns an `Service` object.

## Class: Service

The class `Service` is used to create the `ServiceInterface` instance.

### createObject(objectPath)

* `objectPath` {String} the object path.

Creates the object under `this` service.

### createInterface(name)

* `name` {String} the interface name.

Creates the interface, it returns an `ServiceInterface` instance.

## Class: ServiceInterface

### addMethod(name, opts, handler)

* `name` {String} the method name.
* `opts` {Object} the method options.
  * `opts.in` {Array} the arguments of this method.
  * `opts.out` {Array} the returns of this method.
* `handler` {Function} the handler for this method.

Adds an method to the interface, to start a D-Bus service and add a method:

```js
var dbus = require('dbus');
var service = dbus.registerService('session', 'org.myservice');
var object = service.createObject('/rokid/myservice');
var iface = object.createInterface('rokid.myservice.iface0');

iface.addMethod('ping', { in: [], out: ['s'] }, function(cb) {
  cb(null, 'pong');
});
```

And then we are able to ping this service in another script:

```js
var assert = require('assert');
var dbus = require('dbus').getBus('session');
dbus.getInterface(
  'org.myservice', 
  '/rokid/myservice', 
  'rokid.myservice.iface0', 
  function onResponse(err, iface) {
    iface.ping(function(err, data) {
      assert.equal(data, 'pong');
    });
  }
);
```

### addSignal(name, opts)

* `name` {String} the signal name.
* `opts` {Object} the signal options.
  * `opts.types` {Array} the arguments of this signal.

Add the named signal.

### emit(name, data)

Emits the added signal.

