'use strict';

var nextTickQueue = [];

module.exports.nextTick = function nextTick(callback) {
  if (typeof callback !== 'function') {
    throw new Error('NEXT_TICK_EXPECTED_FUNCTION');
  }
  nextTickQueue.push(arguments);
};

module.exports._onNextTick = function _onNextTick() {
  var i = 0;
  while (i < nextTickQueue.length) {
    var tickArgs = nextTickQueue[i];
    delete nextTickQueue[i];
    var callback = tickArgs[0];
    try {
      switch (tickArgs.length) {
        case 1: callback(); break;
        case 2: callback(tickArgs[1]); break;
        case 3: callback(tickArgs[1], tickArgs[2]); break;
        case 4: callback(tickArgs[1], tickArgs[2], tickArgs[3]); break;
        default:
          callback.apply(undefined, Array.prototype.slice.call(tickArgs, 1));
          break;
      }
    } catch (e) {
      process._onUncaughtException(e);
    }
    ++i;
  }
  nextTickQueue = [];
  return false;
};
