'use strict';
var https = require('https');
var assert = require('assert');
var fs = require('fs');

https.get('https://www.google.com', (res) => {
  assert.fail();
});

fs.access(__filename, function(err) {
  assert.fail();
});

process.exit(0);
