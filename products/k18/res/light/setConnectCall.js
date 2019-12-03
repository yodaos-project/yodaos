'use strict'

var logger = require('logger')('products-light')

module.exports = function setConnectCall (light, data, callback) {
  var leds = light.ledsConfig.leds

  logger.log('------setConnectCall start -------')

  var render = function () {
    light.fill(75, 75, 75)
    light.pixel(0, 0, 0, 0)
    light.render()
    light.requestAnimationFrame(() => {
      render()
    }, 1000)
  }

  light.fill(255, 255, 255)
  light.pixel(0, 0, 0, 0)
  light.render()
  light.requestAnimationFrame(() => {
    render()
  }, 2000)
}
