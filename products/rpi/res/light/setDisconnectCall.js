'use strict'

var logger = require('logger')('products-light')

module.exports = function setDisconnectCall (light, data, callback) {
  var leds = light.ledsConfig.leds
  logger.log('------setDisconnectCall start -------')

  var render = function () {
    light.fill(0, 0, 0)
    light.pixel(0, 255, 0, 0)
    light.render()
    light.requestAnimationFrame(() => {
      light.pixel(0, 0, 0, 0)
    }, 200)
  }

  render()

  light.requestAnimationFrame(() => {
    callback()
  }, 1200)
}
