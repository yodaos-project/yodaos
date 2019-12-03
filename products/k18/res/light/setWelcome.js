'use strict'

module.exports = function (light, data, callback) {
  function render (r, g, b) {
      light.fill(r, g, b)
      light.render()
  }

  light.requestAnimationFrame(() => {
    light.transition({ r: 0, g: 0, b: 0 }, { r: 255, g: 255, b: 255 }, 2400, 80, render)
      .then(() => {
        light.requestAnimationFrame(() => {
          light.transition({ r: 255, g: 255, b: 255 }, { r: 0, g: 0, b: 0 }, 1000, 30, render)
            .then(callback)
        }, 1000)
    })
  }, 1400)
}
