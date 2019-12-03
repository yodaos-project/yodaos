'use strict'
var logger = require('logger')('products-light')

module.exports = function setCall (light, data, callback) {
  var leds = light.ledsConfig.leds - 1
  var pos = 0

  light.clear()
  logger.log('------setCall start -------')

  var circle = function () {
    if (pos < leds) {
      light.pixel(0, 255, 255, 255)
    } else {
      light.pixel(0, 0, 0, 0)
    }
    pos = pos === ((leds) * 2) ? 0 : pos + 1

    light.render()
    light.requestAnimationFrame(() => {
      circle()
    }, 50)
  }

  circle()
  light.requestAnimationFrame(() => {
    callback()
    light.stop()
  }, 2000)
}
