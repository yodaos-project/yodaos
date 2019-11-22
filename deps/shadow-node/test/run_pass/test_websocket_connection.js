'use strict';

var assert = require('assert');
var common = require('../common');
var WebSocket = require('websocket').client;
var socketUrl = 'ws://localhost:8080/websocket/api';
var socketSSLUrl = 'wss://localhost:8088/websocket/api';
var subProtocol = 'echo';
var message = 'this is a websocket test message';

function test(connUrl) {
  console.log(`connect ${connUrl}`);
  var websocket = new WebSocket({});

  websocket.on('connect', common.mustCall((conn) => {
    console.log(`${connUrl} connected`);

    conn.on('message', common.mustCall((inMessage) => {
      assert.strictEqual(message, inMessage.utf8Data);
      conn.close();
    }));

    console.log('send message', message);
    conn.sendUTF(message);
  }));

  websocket.connect(connUrl, subProtocol);
}

test(socketUrl);
test(socketSSLUrl);
