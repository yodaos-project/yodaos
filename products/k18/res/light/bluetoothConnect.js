'use strict'

var logger = require('logger')('products-light')

module.exports = function bluetoothConnect (light, data, callback) {
  logger.log('------bluetoothConnect start -------')
  light.sound(`system://ble_connected.ogg`)

  var render = function (r, g, b) {
      light.fill(r, g, b)
      light.render()
  }

  light.requestAnimationFrame(() => {
    light.transition({ r: 0, g: 0, b: 0 }, { r: 255, g: 255, b: 255 }, 100, 26, render)
      .then(() => {
        light.requestAnimationFrame(() => {
          light.transition({ r: 255, g: 255, b: 255 }, { r: 0, g: 0, b: 0 }, 200, 26, render)
            .then(callback)
        }, 1300)
    })
  }, 50)
}
