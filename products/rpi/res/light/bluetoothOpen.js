'use strict'

module.exports = function bluetoothOpen (light, data, callback) {
  var lightUp = true
  var pos = 0
  var leds = light.ledsConfig.leds - 1
  var render = function () {
    if (lightUp) {
      light.pixel(0, 0, 0, 0)
    } else {
      light.pixel(0, 255, 255, 255)
    }
    light.render()
    pos = pos + 1
    if (pos >= leds) {
      pos = 0
      lightUp = !lightUp
    }
    light.requestAnimationFrame(render, 230)
  }
  render()

  // bluetoothOpen should resume，so has no callback
  return {
    shouldResume: true
  }
}
