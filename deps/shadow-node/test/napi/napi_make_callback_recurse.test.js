'use strict';

var common = require('../common');
var assert = require('assert');
var binding = require('./build/Release/napi_make_callback_recurse.node');
var makeCallback = binding.makeCallback;

// Make sure that using MakeCallback allows the error to propagate.
assert.throws(function() {
  makeCallback({}, function() {
    throw new Error('hi from domain error');
  });
}, /^Error: hi from domain error$/);


// Check the execution order of the nextTickQueue and MicrotaskQueue in
// relation to running multiple MakeCallback's from bootstrap,
// node::MakeCallback() and node::AsyncWrap::MakeCallback().
// TODO(trevnorris): Is there a way to verify this is being run during
// bootstrap?
(function verifyExecutionOrder(arg) {
  var results = [];

  // Processing of the MicrotaskQueue is manually handled by node. They are not
  // processed until after the nextTickQueue has been processed.
  Promise.resolve(1).then(common.mustCall(function() {
    results.push(7);
  }));

  // The nextTick should run after all immediately invoked calls.
  process.nextTick(common.mustCall(function() {
    results.push(3);

    // Run same test again but while processing the nextTickQueue to make sure
    // the following MakeCallback call breaks in the middle of processing the
    // queue and allows the script to run normally.
    process.nextTick(common.mustCall(function() {
      results.push(6);
    }));

    makeCallback({}, common.mustCall(function() {
      results.push(4);
    }));

    results.push(5);
  }));

  results.push(0);

  // MakeCallback is calling the function immediately, but should then detect
  // that a script is already in the middle of execution and return before
  // either the nextTickQueue or MicrotaskQueue are processed.
  makeCallback({}, common.mustCall(function() {
    results.push(1);
  }));

  // This should run before either the nextTickQueue or MicrotaskQueue are
  // processed. Previously MakeCallback would not detect this circumstance
  // and process them immediately.
  results.push(2);

  setImmediate(common.mustCall(function() {
    for (var i = 0; i < results.length; i++) {
      console.log(results);
      assert.strictEqual(results[i], i,
                         `verifyExecutionOrder(${arg}) results: ${results}`);
    }
    if (arg === 1) {
      // The tests are first run on bootstrap during LoadEnvironment() in
      // src/node.cc. Now run the tests through node::MakeCallback().
      setImmediate(function() {
        makeCallback({}, common.mustCall(function() {
          verifyExecutionOrder(2);
        }));
      });
    } else if (arg === 2) {
      // setTimeout runs via the TimerWrap, which runs through
      // AsyncWrap::MakeCallback(). Make sure there are no conflicts using
      // node::MakeCallback() within it.
      setTimeout(common.mustCall(function() {
        // verifyExecutionOrder(3);
      }), 10);
    } else if (arg === 3) {
      // mustCallCheckDomains();
    } else {
      throw new Error('UNREACHABLE');
    }
  }));
}(1));
