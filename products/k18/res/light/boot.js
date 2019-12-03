'use strict'

module.exports = function loading (light, data, callback) {
  var blink = true

  function render () {
    if (blink){
      light.fill(55, 55, 55)
    } else {
      light.fill(0, 0, 0)
    }
    light.render()
    blink = !blink
    light.requestAnimationFrame(render, 237)
  }
  render()
}
