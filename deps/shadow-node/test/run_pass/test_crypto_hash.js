'use strict';
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var crypto = require('crypto');

function test_hash(algorithm, input, output) {
  var hash = crypto.createHash(algorithm);
  assert.strictEqual(hash.update(input).digest('hex'), output);
}

test_hash('md5', 'foobar', '3858f62230ac3c915f300c664312c63f');
test_hash('md5', '中文', 'a7bac2239fcdcb3a067903d8077c4a07');
test_hash('sha256', 'foobar',
          'c3ab8ff13720e8ad9047dd39466b3c8974e592c2fa383d4a3960714caef0c4f2');
test_hash('sha256', '中文',
          '72726d8818f693066ceb69afa364218b692e62ea92b385782363780f47529c21');

function test_update(algorithm, file, expect) {
  // forced to split buffers
  var stream = fs.createReadStream(file, { highWaterMark: 128 });
  var cipher = crypto.createHash(algorithm);
  stream.on('data', (buf) => cipher.update(buf));
  stream.on('end', () => {
    var hash = cipher.digest('hex');
    assert.strictEqual(hash, expect);
  });
}

var file = path.join(__dirname, '..', 'resources', 'tobeornottobe.txt');
test_update('md5', file, '19e03260fad6a77c9f869079cedf664d');
test_update('sha256', file,
            '4e79cc2b93a9233a8b77c60971231431502b22c7cee1e2ebc10349a28d1852fa');
