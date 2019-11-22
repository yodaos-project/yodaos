'use strict';

asyncFunction();
function asyncFunction() {
  return Promise.reject(new Error('foobar'));
}
