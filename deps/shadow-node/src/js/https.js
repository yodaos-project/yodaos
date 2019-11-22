/* Copyright 2017-present Samsung Electronics Co., Ltd. and other contributors
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

var client = require('https_client');
var util = require('util');
var url = require('url');

var HttpAgent = require('http_agent').Agent;
var ClientRequest = exports.ClientRequest = client.ClientRequest;

exports.request = function(options, cb) {
  if (typeof options === 'string') {
    options = url.parse(options);
  }
  return new ClientRequest(options, cb);
};

exports.METHODS = client.METHODS;

exports.get = function(options, cb) {
  var req = exports.request(options, cb);
  req.end();
  return req;
};

function Agent(options) {
  if (!(this instanceof Agent))
    return new Agent(options);

  HttpAgent.call(this, options);
  this.defaultPort = 443;
  this.protocol = 'https:';
  this.maxCachedSessions = this.options.maxCachedSessions;
  if (this.maxCachedSessions === undefined)
    this.maxCachedSessions = 100;

  this._sessionCache = {
    map: {},
    list: []
  };
}
util.inherits(Agent, HttpAgent);

exports.Agent = Agent;
exports.globalAgent = new Agent();
