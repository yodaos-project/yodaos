'use strict';

var detect = require('es-feature-detection');
var syntax = detect.syntax();
var builtins = detect.builtins();

function report(obj, version) {
  console.log(`${obj.type}: ${version}`);
  return _report(obj[version], '  ');
}

function _report(obj, space) {
  var keys = Object.keys(obj);
  keys.forEach((name) => {
    if (name === '__all')
      return;
    var support = obj[name];
    var prefix = '\u001b';

    if (support && support.__all !== undefined) {
      return _report(support, space + space);
    } else if (support) {
      console.log(`${prefix}[32m${space}${name}: ${support}${prefix}[0m`);
    } else {
      console.log(`${prefix}[90m${space}${name}: ${support}${prefix}[0m`);
    }
  });
  console.log('');
}

syntax.type = 'syntax';
builtins.type = 'builtins';

report(syntax, 'es2015');
report(syntax, 'es2016');
report(builtins, 'es2015');
report(builtins, 'es2016');
