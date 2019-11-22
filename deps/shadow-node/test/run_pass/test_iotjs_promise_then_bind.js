'use strict';

var assert = require('assert');

class TestObj {
  constructor() {}
  then() {
    assert(this === obj);
  }
}

var obj = new TestObj();
Promise.resolve().then(() => obj);
