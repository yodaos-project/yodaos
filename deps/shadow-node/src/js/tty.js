'use strict';

exports.isatty = function(fd) {
  return native.isatty(fd || 0);
};
