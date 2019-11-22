/* Copyright 2015-present Samsung Electronics Co., Ltd. and other contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
'use strict';

var assert = require('assert');
var Server = require('http_server').Server;
var http = require('http');


var responseCheck = '';
var connectionEvent = 0;
var serverCloseEvent = 0;
var requestEvent = 0;
var responseEvent = 0;
var socketEvent = 0;
// server side code
// server will return the received msg from client
// and shutdown
var server = http.createServer(function(req, res) {

  var body = '';

  req.on('data', function(chunk) {
    body += chunk;
  });

  function endHandler() {
    res.writeHead(200, { 'Connection': 'close',
                         'Content-Length': body.length
    });
    res.write(body);
    res.end(function() {
      if (body === 'close server') {
        server.close();
      }
    });
  }

  req.on('end', endHandler);

});

server.on('request', function() {
  requestEvent++;
});

server.on('connection', function() {
  connectionEvent++;
});

server.on('close', function() {
  serverCloseEvent++;
});

server.listen(3001, 3);


// client side code
// 1. send POST req to server and check response msg
// 2. send GET req to server and check response msg
// 3. send 'close server' msg

// 1. POST req
var msg = 'http request test msg';
var options = {
  method: 'POST',
  port: 3001,
  headers: { 'Content-Length': msg.length }
};


function postResponseHandler(res) {
  var res_body = '';

  assert.strictEqual(200, res.statusCode);
  function endHandler() {
    assert.strictEqual(msg, res_body);
    responseCheck += '1';
  }
  res.on('end', endHandler);

  res.on('data', function(chunk) {
    res_body += chunk.toString();
  });
}

var req = http.request(options, postResponseHandler);
req.on('response', function() {
  responseEvent++;
});
req.on('socket', function() {
  socketEvent++;
});
req.write(msg);
req.end();


// 2. GET req
options = {
  method: 'GET',
  port: 3001
};

var getResponseHandler = (res) => {
  var res_body = '';

  assert.strictEqual(200, res.statusCode);

  var endHandler = () => {
    // GET msg, no received body
    assert.strictEqual('', res_body);
    responseCheck += '2';
  };
  res.on('end', endHandler);

  res.on('data', function(chunk) {
    res_body += chunk.toString();
  });
};


var getReq = http.request(options, getResponseHandler);
getReq.on('response', function() {
  responseEvent++;
});
getReq.on('socket', function() {
  socketEvent++;
});
getReq.end();


// 3. close server req
var finalMsg = 'close server';
var finalOptions = {
  method: 'POST',
  port: 3001,
  headers: { 'Content-Length': finalMsg.length }
};

var finalResponseHandler = (res) => {
  var res_body = '';

  assert.strictEqual(200, res.statusCode);

  var endHandler = () => {
    assert.strictEqual(finalMsg, res_body);
    responseCheck += '3';
  };
  res.on('end', endHandler);

  res.on('data', function(chunk) {
    res_body += chunk.toString();
  });
};

var finalReq = http.request(finalOptions, finalResponseHandler);
finalReq.on('response', function() {
  responseEvent++;
});
finalReq.on('socket', function() {
  socketEvent++;
});
finalReq.write(finalMsg);
finalReq.end();

// Create server without requestListener.
var server2 = http.createServer();

// Create server instance without new keyword.
var server3 = Server(function(request, response) {});

process.on('exit', function() {
  assert.strictEqual(responseCheck.length, 3);
  assert.strictEqual(connectionEvent, 3);
  assert.strictEqual(serverCloseEvent, 1);
  assert.strictEqual(requestEvent, 3);
  assert.strictEqual(responseEvent, 3);
  assert.strictEqual(socketEvent, 3);
  assert.strictEqual(server2 instanceof Server, true);
  assert.strictEqual(server3 instanceof Server, true);
});
