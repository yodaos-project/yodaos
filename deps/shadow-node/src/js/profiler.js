'use strict';

/**
 * @class Snapshot
 */
function Snapshot(path) {
  native.takeSnapshot(path);
}

/**
 * @method getHeader
 */
Snapshot.prototype.getHeader = function() {
  // TODO
  throw new Error('not implemented');
};

/**
 * @method compare
 */
Snapshot.prototype.compare = function(snapshot) {
  // TODO
  throw new Error('not implemented');
};

/**
 * @method export
 */
Snapshot.prototype.export = function(cb) {
  // TODO
  throw new Error('not implemented');
};

/**
 * @method serialize
 */
Snapshot.prototype.serialize = function() {
  // TODO
  throw new Error('not implemented');
};

/**
 * @class Profile
 */
function Profile() {
  // TODO
}

/**
 * @method getHeader
 */
Profile.prototype.getHeader = function() {
  throw new Error('not implemented');
};

/**
 * @method delete
 */
Profile.prototype.delete = function() {
  throw new Error('not implemented');
};

/**
 * @method export
 */
Profile.prototype.export = function(cb) {
  throw new Error('not implemented');
};

/**
 * @method getHeader
 */
function takeSnapshot(path) {
  if (typeof path !== 'string') {
    path = `${process.cwd()}/Profile-${Date.now()}`;
  }
  return new Snapshot(path);
}

/**
 * startProfiling()
 * startProfiling(path)
 * startProfiling(duration)
 * startProfiling(path, duration)
 */
function startProfiling() {
  var path = `${process.cwd()}/Profile-${Date.now()}`;
  var duration = -1;

  if (arguments.length === 1) {
    if (typeof (arguments[0]) === 'number') {
      duration = arguments[0];
    } else if (typeof (arguments[0]) === 'string') {
      path = arguments[0];
    }
  } else if (arguments.length === 2) {
    path = arguments[0];
    duration = arguments[1];
  }

  native.startProfiling(path, duration);
}

function stopProfiling() {
  native.stopProfiling();
  return new Profile();
}

exports.takeSnapshot = takeSnapshot;
exports.startProfiling = startProfiling;
exports.stopProfiling = stopProfiling;
