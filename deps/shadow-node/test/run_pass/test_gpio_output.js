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

var assert = require('assert');
var Gpio = require('gpio');
var gpio = new Gpio();

var LED_ON = true;
var LED_OFF = false;
var pin, mode;
var gpio20;

if (process.platform === 'linux') {
  pin = 20;
  mode = gpio.MODE.NONE;
} else if (process.platform === 'nuttx') {
  pin = require('stm32f4dis').pin.PA10;
  mode = gpio.MODE.PUSHPULL;
} else if (process.platform === 'tizenrt') {
  pin = 41;
  mode = gpio.MODE.NONE;
} else {
  assert.fail();
}

test1();

gpio20 = gpio.open({
  pin: pin,
  direction: gpio.DIRECTION.OUT,
  mode: mode
}, test2);

function test1() {
  assert.notStrictEqual(gpio.DIRECTION.IN, undefined);
  assert.notStrictEqual(gpio.DIRECTION.OUT, undefined);
  assert.notStrictEqual(gpio.MODE.NONE, undefined);
  if (process.platform === 'nuttx') {
    assert.notStrictEqual(gpio.MODE.PULLUP, undefined);
    assert.notStrictEqual(gpio.MODE.PULLDOWN, undefined);
    assert.notStrictEqual(gpio.MODE.FLOAT, undefined);
    assert.notStrictEqual(gpio.MODE.PUSHPULL, undefined);
    assert.notStrictEqual(gpio.MODE.OPENDRAIN, undefined);
  }
}

// turn on LED for 3000ms
function test2(err) {
  assert.strictEqual(err, null);

  gpio20.write(LED_ON, function(writeErr) {
    assert.strictEqual(writeErr, null);
    console.log('gpio write');

    gpio20.read(function(readErr, value) {
      assert.strictEqual(readErr, null);
      console.log('gpio read:', value);
      assert.strictEqual(LED_ON, value);

      setTimeout(function() {
        gpio20.writeSync(LED_OFF);
        var value = gpio20.readSync();
        console.log('gpio read:', value);
        assert.strictEqual(LED_OFF, value);
        gpio20.close();
        console.log('finish test');
      }, 3000);
    });
  });
}
