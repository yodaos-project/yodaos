'use strict';

setInterval(() => {
  process.send(Math.random());
}, 0);
