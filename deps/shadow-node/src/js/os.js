'use strict';

var constants = require('constants');

exports.hostname = function() {
  return native.getHostname();
};

exports.uptime = function() {
  return native.getUptime();
};

exports.totalmem = function() {
  return native.getTotalMem();
};

exports.freemem = function() {
  return native.getFreeMem();
};

exports.getPriority = function(pid) {
  if (pid === undefined) {
    pid = 0;
  } else {
    pid = Number(pid);
  }

  var err = native.getPriority(pid);
  if (err instanceof Error) {
    throw err;
  }
  return err;
};

exports.setPriority = function(pid, priority) {
  if (priority === undefined) {
    priority = pid;
    pid = 0;
  } else {
    pid = Number(pid);
  }

  if (typeof priority !== 'number') {
    throw new TypeError('priority must be a number');
  }
  if (priority > constants.UV_PRIORITY_LOW ||
    priority < constants.UV_PRIORITY_HIGHEST) {
    throw new RangeError(
      'The value of "priority" is out of range. It must be' +
      `> ${constants.UV_PRIORITY_HIGHEST} && < ${constants.UV_PRIORITY_LOW}.`);
  }
  var err = native.setPriority(pid, priority);
  if (err instanceof Error) {
    throw err;
  }
};

exports.platform = function() {
  return process.platform;
};

exports.release = function() {
  return native._getOSRelease();
};

exports.networkInterfaces = function() {
  var list = native.getInterfaceAddresses();
  var interfaces = {};
  for (var i = 0; i < list.length; i++) {
    var item = list[i];
    var name = item.name;
    if (!interfaces[name]) {
      interfaces[name] = [];
    }
    interfaces[name].push({
      address: item.address,
      netmask: item.netmask,
      family: item.family,
      broadcast: item.broadcast,
      mac: item.mac,
    });
  }
  return interfaces;
};

Object.defineProperty(exports, 'EOL', {
  get: function() {
    return '\n';
  }
});
