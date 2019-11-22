'use strict';

var net = require('net');
var Pipe = require('pipe_wrap').Pipe;

var handle = new Pipe();
handle.open(0);

var socket = new net.Socket({ handle: handle });
socket.end();
