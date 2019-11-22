'use strict';

var assert = require('assert');
var http = require('http');

var responded = false;
var server = http.createServer((req, res) => {
  var clientData = '';
  assert.strictEqual(req.method, 'POST');
  req.on('data', (chunk) => clientData += chunk.toString());
  req.on('end', () => {
    assert.strictEqual(clientData, 'foobar');
    res.setHeader('Transfer-Encoding', 'chunked');
    res.write('ok');
    res.end();
    server.close();
  });
}).listen(0, () => {
  var request = http.request({
    port: server.address().port,
    encoding: 'utf8',
    method: 'POST',
    headers: {
      'Transfer-Encoding': 'chunked',
    },
  }, (clientResponse) => {
    var data = '';
    assert.strictEqual(clientResponse.statusCode, 200);
    clientResponse.on('data', (chunk) => data += chunk.toString());
    clientResponse.on('end', () => {
      assert.strictEqual(data, 'ok');
      responded = true;
    });
  });
  request.write('foobar');
  request.end();
});

process.once('exit', () => {
  assert(responded);
});
