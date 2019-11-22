'use strict';

var fs = require('fs');
var path = require('path');

// Create an object of all benchmark scripts
var benchmarks = {};
fs.readdirSync(__dirname)
  .filter(function(name) {
    return fs.statSync(path.resolve(__dirname, name)).isDirectory();
  })
  .forEach(function(category) {
    benchmarks[category] = fs.readdirSync(path.resolve(__dirname, category))
      .filter((filename) => filename[0] !== '.' && filename[0] !== '_');
  });

function CLI(usage, settings) {
  if (!(this instanceof CLI))
    return new CLI(usage, settings);

  if (process.argv.length < 3) {
    this.abort(usage); // abort will exit the process
  }

  this.usage = usage;
  this.optional = {};
  this.items = [];

  settings.arrayArgs.forEach((argName) => {
    this.optional[argName] = [];
  });

  var currentOptional = null;
  var mode = 'both'; // possible states are: [both, option, item]

  process.argv.slice(2).forEach((arg) => {
    if (arg === '--') {
      // Only items can follow --
      mode = 'item';
    } else if (['both', 'option'].indexOf(mode) !== -1 && arg[0] === '-') {
      // Optional arguments declaration

      if (arg[1] === '-') {
        currentOptional = arg.slice(2);
      } else {
        currentOptional = arg.slice(1);
      }

      if (settings.boolArgs &&
        settings.boolArgs.indexOf(currentOptional) !== -1) {
        this.optional[currentOptional] = true;
        mode = 'both';
      } else {
        // expect the next value to be option related (either -- or the value)
        mode = 'option';
      }
    } else if (mode === 'option') {
      // Optional arguments value

      if (settings.arrayArgs.indexOf(currentOptional) !== -1) {
        this.optional[currentOptional].push(arg);
      } else {
        this.optional[currentOptional] = arg;
      }

      // the next value can be either an option or an item
      mode = 'both';
    } else if (['both', 'item'].indexOf(mode) !== -1) {
      // item arguments
      this.items.push(arg);

      // the next value must be an item
      mode = 'item';
    } else {
      // Bad case, abort
      this.abort(usage);

    }
  });
}
module.exports = CLI;

CLI.prototype.abort = function(msg) {
  console.error(msg);
  process.exit(1);
};

CLI.prototype.benchmarks = function() {
  var paths = [];
  var filter = this.optional.filter || false;

  this.items.forEach((category) => {
    if (benchmarks[category] === undefined)
      return;

    benchmarks[category].forEach((scripts) => {
      if (filter && scripts.lastIndexOf(filter) === -1)
        return;
      paths.push(path.join(category, scripts));
    });
  });

  return paths;
};
