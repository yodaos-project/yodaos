'use strict'
var logger = require('logger')('products-light')

module.exports = function setMuted (light, data, callback) {
  var muted = !!(data && data.muted)
  var leds = light.ledsConfig.leds

  logger.log('------setMuted start -------')

  light.clear()
  if (!muted) {
    light.sound(`system://mic_open.wav`)
    light.render()
    callback()
  } else {
    light.sound('system://mic_close_tts.wav')
    light.pixel(0, 255, 255, 255)
    light.render()
    return {
      shouldResume: true
    }
  }
}
