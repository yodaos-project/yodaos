'use strict'

var logger = require('logger')('products-light')

module.exports = function setSpeaking (light, data, callback) {
  var base = 120
  var adjustable = 135
  var final
  logger.log('------setSpeaking start -------')

  var render = function () {
    final = base + Math.floor(Math.random() * adjustable)
    light.fill(final, final, final)
    light.render()
    light.requestAnimationFrame(() => {
      render()
    }, 45)
  }
  render()

  return {
    shouldResume: true
  }
}
