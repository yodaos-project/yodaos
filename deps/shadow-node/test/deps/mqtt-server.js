'use strict';
var path = require('path');
var mosca = require('mosca');
var keyPath = path.resolve(__dirname, './test-key.pem');
var certPath = path.resolve(__dirname, './test-cert.pem');

var port = 1883;
var idx = process.argv.indexOf('--port');
if (idx > -1) {
  port = Number(process.argv[idx + 1]);
}

var options;
if (process.argv.indexOf('--ssl') > -1) {
  options = {
    interfaces: [
      { type: 'mqtts', port: port },
    ],
    credentials: {
      keyPath: keyPath,
      certPath: certPath,
    },
  };
} else {
  options = {
    interfaces: [
      { type: 'mqtt', port: port },
    ],
  };
}

var server = new mosca.Server(options);

server.on('clientConnected', function(client) {
  console.log('client connected', client.id);
});

// fired when a message is received
server.on('published', function(packet, client) {
  console.log('Published', packet);
});

server.on('ready', function() {
  console.log(`${options.interfaces[0].type} is running on port ${port}`);
});
