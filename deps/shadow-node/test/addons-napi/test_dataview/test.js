'use strict';
var common = require('../../common');
var assert = require('assert');

// Testing api calls for arrays
var test_dataview = require(`./build/Release/test_dataview.node`);

// Test for creating dataview
{
  var buffer = new ArrayBuffer(128);
  var template = Reflect.construct(DataView, [buffer]);

  var theDataview = test_dataview.CreateDataViewFromJSDataView(template);
  assert.ok(theDataview instanceof DataView,
            `Expect ${theDataview} to be a DataView`);
}

// Test for creating dataview with invalid range
{
  var buffer = new ArrayBuffer(128);
  assert.throws(() => {
    test_dataview.CreateDataView(buffer, 10, 200);
  }, RangeError);
}
