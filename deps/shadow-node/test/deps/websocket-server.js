'use strict';
var WebSocketServer = require('websocket').server;
var http = require('http');
var fs = require('fs');
var https = require('https');
var path = require('path');
var keyPath = path.resolve(__dirname, './test-key.pem');
var certPath = path.resolve(__dirname, './test-cert.pem');

var port = 8080;
var idx = process.argv.indexOf('--port');
if (idx > -1) {
  port = Number(process.argv[idx + 1]);
}
var agent;
var options;
if (process.argv.indexOf('--ssl') > -1) {
  agent = https;
  options = {
    key: fs.readFileSync(keyPath),
    cert: fs.readFileSync(certPath)
  };
} else {
  agent = http;
}

var httpServer = agent.createServer(options, function(request, response) {
  response.writeHead(404);
  response.end();
});
httpServer.listen(port, function() {
  console.log(`websocket is listening on port ${port}`);
});

var wsServer = new WebSocketServer({
  httpServer: [httpServer],
  autoAcceptConnections: false
});

wsServer.on('request', function(request) {
  var connection = request.accept('echo', request.origin);
  console.log((new Date()) + ' Connection accepted.');
  connection.on('message', function(message) {
    if (message.type === 'utf8') {
      console.log('UTF8 Message of ' + message.utf8Data.length + ' bytes');
      connection.sendUTF(message.utf8Data);
    } else if (message.type === 'binary') {
      console.log('Binary Message of ' + message.binaryData.length + ' bytes');
      connection.sendBytes(message.binaryData);
    }
  });
  connection.on('close', function(reasonCode, description) {
    console.log(' Peer ' + connection.remoteAddress + ' disconnected.');
  });
});
