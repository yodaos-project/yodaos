'use strict';

function throwError2() {
  throw new Error('dump2');
}

module.exports = function main2() {
  throwError2();
};
