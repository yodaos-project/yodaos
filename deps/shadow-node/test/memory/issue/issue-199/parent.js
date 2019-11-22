'use strict';

var path = require('path');
var child = require('child_process').fork(path.join(__dirname, '/child.js'), {
  env: {
    isSubprocess: 'true',
  },
});

child.on('message', (data) => {
  // console.log(data.toString());
});
