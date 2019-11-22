'use strict';

function throwError() {
  throw new Error('dump1');
}

module.exports = function main() {
  throwError();
};
