'use strict'

var logger = require('logger')('products-light')

module.exports = function setupNetworkSuccess (light, data, callback) {
  logger.log('------setupNetworkSuccess start -------')

  function render (r, g, b) {
      light.fill(r, g, b)
      light.render()
  }

        light.requestAnimationFrame(() => {
    light.transition({ r: 0, g: 0, b: 0 }, { r: 255, g: 255, b: 255 }, 200, 26, render)
      .then(() => {
        light.requestAnimationFrame(() => {
          light.transition({ r: 255, g: 255, b: 255 }, { r: 0, g: 0, b: 0 }, 200, 26, render)
            .then(callback)
        }, 6500)
    })
  }, 50)

  light.sound(`system://startup0.ogg`)
}
