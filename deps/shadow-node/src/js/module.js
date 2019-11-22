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

var Native = require('native');
var fs = Native.require('fs');
var path = Native.require('path');
var debug = Native.require('debug')('module');

function iotjs_module_t(id, parent) {
  this.id = id;
  this.exports = {};
  this.filename = null;
  this.parent = parent;
}

module.exports = iotjs_module_t;


iotjs_module_t.cache = {};
iotjs_module_t.wrapper = Native.wrapper;
iotjs_module_t.wrap = Native.wrap;
iotjs_module_t.curr = null;

var mainModule = {
  id: '.',
  exports: {},
  parent: null,
  filename: '',
};

var cwd;
try {
  cwd = process.cwd();
} catch (e) { }

var moduledirs = [];
if (process.env.NODE_PRIORITIZED_PATH) {
  moduledirs.push(`${process.env.NODE_PRIORITIZED_PATH}/`);
}
if (cwd) {
  moduledirs.push(`${cwd}/node_modules/`);
}
if (process.env.NODE_PATH) {
  moduledirs.push(`${process.env.NODE_PATH}/`);
}
if (process.env.HOME) {
  moduledirs.push(`${process.env.HOME}/node_modules/`);
}

function tryPath(modulePath, ext) {
  return iotjs_module_t.tryPath(modulePath + ext) ||
         iotjs_module_t.tryPath(modulePath);
}


iotjs_module_t.tryPath = function(path) {
  try {
    debug(`try load path ${path}`);
    var stats = fs.statSync(path);
    if (stats && !stats.isDirectory()) {
      return path;
    }
  } catch (ex) { }

  return false;
};


iotjs_module_t.resolveDirectories = function(id, parent) {
  var dirs = [];
  if (parent) {
    var start = path.dirname(parent.filename);
    var parts = start.split('/');
    do {
      var last = parts[parts.length - 1];
      dirs.push(start + 'node_modules/');
      start = start.replace(new RegExp(last + '/?$'), '');
      parts.length = parts.length - 1;
    } while (parts.length > 0);
  }
  dirs = dirs.concat(moduledirs);

  if (parent) {
    if (!parent.dirs) {
      parent.dirs = [];
    }
    dirs = parent.dirs.concat(dirs);
  }
  return dirs;
};


iotjs_module_t.resolveFilepath = function(id, directories) {
  for (var i = 0; i < directories.length; i++) {
    var dir = directories[i];
    var modulePath = dir + id;

    if (modulePath[0] !== '/') {
      modulePath = process.cwd() + '/' + modulePath;
    }

    if (process.platform === 'tizenrt' &&
        (modulePath.indexOf('../') !== -1 || modulePath.indexOf('./') !== -1)) {
      modulePath = iotjs_module_t.normalizePath(modulePath);
    }

    var filepath = this._resolveFilepath(modulePath, false);
    if (filepath) {
      return filepath;
    }
  }
  return false;
};


iotjs_module_t._resolveFilepath = function(id, root) {
  var modulePath = root ? path.join(root, id) : id;
  var filepath;
  var ext = '.js';

  // id[.ext]
  if (filepath = tryPath(modulePath, ext)) {
    return filepath;
  }

  // id/index[.ext]
  if (filepath = tryPath(modulePath + '/index', ext)) {
    return filepath;
  }

  // 3. package path id/
  var jsonpath = modulePath + '/package.json';
  filepath = iotjs_module_t.tryPath(jsonpath);
  if (filepath) {
    var pkgSrc = process.readSource(jsonpath);
    var pkgMainFile = JSON.parse(pkgSrc).main;

    // pkgmain[.ext]
    if (filepath = tryPath(modulePath + '/' + pkgMainFile, ext)) {
      return filepath;
    }
  }
};


iotjs_module_t.resolveModPath = function(id, parent) {
  if (process.builtin_modules[id]) {
    return id;
  }

  if (parent != null && id === parent.id) {
    return false;
  }

  var filepath = false;
  if (id[0] === '/') {
    filepath = iotjs_module_t._resolveFilepath(id, false);
  } else if (parent === null) {
    filepath = iotjs_module_t._resolveFilepath(id, cwd);
  } else if (id[0] === '.') {
    var root = path.dirname(parent.filename);
    filepath = iotjs_module_t._resolveFilepath(id, root);
  } else {
    var dirs = iotjs_module_t.resolveDirectories(id, parent);
    filepath = iotjs_module_t.resolveFilepath(id, dirs);
  }

  if (filepath &&
    (filepath.indexOf('./') > 0 || filepath.indexOf('../') > 0)) {
    return iotjs_module_t.normalizePath(filepath);
  }
  return filepath;
};


iotjs_module_t.normalizePath = function(path) {
  var beginning = '';
  if (path.indexOf('/') === 0) {
    beginning = '/';
  }

  var input = path.split('/');
  var output = [];
  while (input.length > 0) {
    if (input[0] === '.' || (input[0] === '' && input.length > 1)) {
      input.shift();
      continue;
    }
    if (input[0] === '..') {
      input.shift();
      if (output.length > 0 && output[output.length - 1] !== '..') {
        output.pop();
      } else {
        throw new Error('Requested path is below root: ' + path);
      }
      continue;
    }
    output.push(input.shift());
  }
  return beginning + output.join('/');
};


iotjs_module_t.load = function(id, parent, isMain) {
  var modPath = iotjs_module_t.resolveModPath(id, parent);
  var cachedModule = iotjs_module_t.cache[modPath];
  if (cachedModule) {
    iotjs_module_t.curr = modPath;
    return cachedModule.exports;
  }

  if (process.builtin_modules[id]) {
    iotjs_module_t.curr = id;
    return Native.require(id);
  }

  var module = new iotjs_module_t(id, parent);
  if (!modPath) {
    throw new Error('Module not found: ' + id);
  }

  var stat = process._loadstat();
  var startedAt;
  if (stat) {
    startedAt = Date.now();
  }

  module.filename = modPath;
  module.dirs = [modPath.substring(0, modPath.lastIndexOf('/') + 1)];
  iotjs_module_t.cache[modPath] = module;
  iotjs_module_t.curr = modPath;

  if (isMain) {
    mainModule = module;
    module.id = '.';
  }

  var ext = modPath.substr(modPath.lastIndexOf('.') + 1);
  if (ext === 'jsc') {
    module.compile(true);
  } else if (ext === 'json') {
    var source = process.readSource(modPath);
    module.exports = JSON.parse(source);
  } else if (ext === 'node') {
    var native = process.openNativeModule(module.filename);
    module.exports = native;
  } else {
    /** Treat any other file as js file */
    module.compile();
  }

  if (stat) {
    var relPath = modPath.replace(cwd, '');
    var consume = Math.floor(Date.now() - startedAt);
    console.log(`load "${relPath}" ${consume}ms`);
  }

  return module.exports;
};

iotjs_module_t.prototype.compile = function(snapshot) {
  var __filename = this.filename;
  var __dirname = path.dirname(__filename);
  var fn;
  if (!snapshot) {
    fn = process.compile(__filename);
  } else {
    fn = process.compileSnapshot(__filename);
    if (typeof fn !== 'function')
      throw new TypeError('Invalid snapshot file.');
  }

  var _require = this.require.bind(this);
  _require.main = mainModule;
  _require.cache = iotjs_module_t.cache;

  fn.apply(this.exports, [
    this.exports,             // exports
    _require,                 // require
    this,                     // module
    undefined,                // native
    __filename,               // __filename
    __dirname                 // __dirname
  ]);
};

// FIXME(Yorkie): dont use it
// function makeSnapshot(id) {
//   var filename = iotjs_module_t.resolveModPath(id, null);
//   if (!filename) {
//     throw new Error('Module not found: ' + id);
//   }
//   var source = process.readSource(filename);
//   return process.makeSnapshot(filename, source);
// }

iotjs_module_t.runMain = function() {
  if (process.debuggerWaitSource) {
    var fn = process.debuggerSourceCompile();
    fn.call();
  } else {
    iotjs_module_t.load(process.argv[1], null, true);
  }
  while (process._onNextTick());
};

iotjs_module_t.prototype.require = function(id) {
  return iotjs_module_t.load(id, this);
};
