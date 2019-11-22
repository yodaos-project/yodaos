'use strict';

var help = `
Usage:

  Run https server:
  $ node http_server.js --ssl

  Specify a port to listen rather than default 4443:
  $ node http_server.js --port 1234
`;

var http = require('http');
var https = require('https');
var fs = require('fs');
var path = require('path');

function handleRequest(req, res) {
  // Process Post Request
  if (req.method === 'POST') {

    var data = '';

    req.on('data', function(chunk) {
      data += chunk;
    });

    req.on('end', function() {
      res.end(data);
    });
  } else { // Send a simple response
    res.end('Everything works');
  }
}

function main(options) {
  var handler = http.createServer;
  if (options.ssl) {
    handler = https.createServer;
    options = Object.assign({}, options, {
      key: fs.readFileSync(path.join(__dirname, 'certs', 'server.key')),
      cert: fs.readFileSync(path.join(__dirname, 'certs', 'server.crt')),
    });
  }

  var PORT = options.port;

  console.log(`-> Starting ${options.ssl ? 'https' : 'http'} \
server at port ${PORT}`);

  // Create a server
  var server = handler(options, handleRequest);

  // Start server
  server.listen(PORT, function() {
    console.log(`-> Server listening on: \
${options.ssl ? 'https' : 'http'}://localhost:${PORT}`);
  });
}

if (require.main === module) {
  var options = {
    ssl: false,
    port: 4443,
  };
  if (process.argv.indexOf('--help') > -1) {
    console.log(help);
    process.exit(0);
  }
  if (process.argv.indexOf('--ssl') > -1) {
    options.ssl = true;
  }
  var idx = process.argv.indexOf('--port');
  if (idx > -1) {
    options.port = Number(process.argv[idx + 1]);
  }
  main(options);
}
