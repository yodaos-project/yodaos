'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [128],
});

var urlObject = require('./url.json');

function appendUrl(urlObject) {
  var protocol = urlObject.protocol;
  var hostname = urlObject.hostname;
  var pathname = urlObject.pathname;
  var url = `${protocol}://${hostname}/${pathname}`;
  if (urlObject.query != null && Object.keys(urlObject.query).length > 0) {
    var query = '?';
    for (var key in urlObject.query) {
      var value = urlObject.query[key];
      query += `&${key}=${value}`;
    }
    url += query;
  }
  return url;
}

function main(opts) {
  var n = opts.n;
  bench.start();
  for (var i = 0; i < n; ++i)
    appendUrl(urlObject);
  bench.end(n);
}
