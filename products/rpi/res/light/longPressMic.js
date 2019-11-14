'use strict'

module.exports = function longPressMic (light, data, callback) {
  var render = function () {
    var from = { r: 52, g: 52, b: 52 }
    var to = { r: 255, g: 255, b: 255 }
    light.transition(from, to, 2500, 25, (r, g, b) => {
      for (var i = 0; i < 3; i++) {
        light.pixel(0, r, g, b)
      }
      light.render()
    }).then(() => {
      light.requestAnimationFrame(callback, 2000)
    })
  }
  render()
}
