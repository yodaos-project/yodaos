'use strict';

var EventEmitter = require('events').EventEmitter;
var util = require('util');

function Agent(options) {
  if (!(this instanceof Agent))
    return new Agent(options);

  EventEmitter.call(this);
  this.defaultPort = 80;
  this.protocol = 'http:';
  this.options = Object.assign({}, options);

  // don't confuse net and make it think that we're connecting to a pipe
  this.options.path = null;
  this.requests = {};
  this.sockets = {};
  this.freeSockets = {};
  this.keepAliveMsecs = this.options.keepAliveMsecs || 1000;
  this.keepAlive = this.options.keepAlive || false;
  this.maxSockets = this.options.maxSockets || Agent.defaultMaxSockets;
  this.maxFreeSockets = this.options.maxFreeSockets || 256;
}

util.inherits(Agent, EventEmitter);
Agent.defaultMaxSockets = Infinity;

module.exports = {
  Agent: Agent,
  globalAgent: new Agent(),
};
