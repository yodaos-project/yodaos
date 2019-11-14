'use strict'

var logger = require('logger')('products-light')

module.exports = function setVolume (light, data, callback) {
  var pos = Math.floor((data.volume / 100) * (light.ledsConfig.leds - 1))
  var action = data.action
  var repeat = 0
  logger.log('------setVolume start -------')

  light.clear()
  var arrowRender = function () {
    if (action === 'increase') {
      light.pixel(0, 255, 255, 255)
    } else if (action === 'decrease') {
      light.pixel(0, 130, 130, 130)
    }

    light.render()
    light.requestAnimationFrame(() => {
      light.pixel(0, 75, 75, 75)
      if (action === 'increase') {
        light.pixel(0, 255, 255, 255)
      } else if (action === 'decrease') {
        light.pixel(0, 75, 75, 75)
      }
      light.render()
    }, 400)

    light.requestAnimationFrame(() => {
      light.fill(0, 0, 0)
      light.render()
      callback()
    }, 800)
  }

  function render (r, g, b) {
      light.fill(r, g, b)
      light.render()
  }

  var maxRender = function () {
    light.transition({ r: 0, g: 0, b: 0 }, { r: 255, g: 255, b: 255 }, 100, 26, render)
      .then(() => light.transition({ r: 255, g: 255, b: 255 }, { r: 75, g: 75, b: 75 }, 100, 26, render))
      .then(() => light.transition({ r: 75, g: 75, b: 75 }, { r: 255, g: 255, b: 255 }, 100, 26, render))
      .then(() => light.transition({ r: 255, g: 255, b: 255 }, { r: 0, g: 0, b: 0 }, 100, 26, render))
      .then(() => {
                    light.clear()
                    light.render()
                    callback()
      })
  }

  pos = Math.max(1, pos)
  var options = {ignore:true}
  light.sound('system://volume.wav', '', options)

  if (data.volume === 100 || data.volume === 0) {
    maxRender()
  } else {
    if (action != null) {
      arrowRender(repeat)
    } else {
      for (var i = 0; i < pos; i++) {
        light.pixel(0, 255, 255, 255)
      }
      light.render()
      callback()
    }
  }
}
