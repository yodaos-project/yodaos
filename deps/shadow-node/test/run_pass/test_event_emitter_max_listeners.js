'use strict';

var EventEmitter = require('events');
var events = new EventEmitter();
var common = require('../common');

events.on('maxListeners', common.mustCall());

var throwsObjs = [NaN, -1, 'and even this'];

var obj;
for (var i = 0; i < throwsObjs.length; i++) {
  obj = throwsObjs[i];

  common.expectsError(
    function() {
      events.setMaxListeners(obj);
    },
    'maxListeners must be a non-negative number value'
  );

  common.expectsError(
    function() {
      events.setDefaultMaxListeners(obj);
    },
    'defaultMaxListeners must be a non-negative number value'
  );
}

events.emit('maxListeners');
