'use strict'

var logger = require('logger')('products-light')

module.exports = function pickup (light, data, callback) {
  var count = 0
  var colorBright = { r: 255, g: 255, b: 255 }
  var colorDark = { r: 50, g: 50, b: 50 }
  var from = colorBright
  var to = colorDark

  logger.log('------awake start -------')

  var render = function (r, g, b) {
    light.fill(r, g, b)
    light.render()
  }

  function delayAndShutdown () {
    light.requestAnimationFrame(() => {
      if (count % 2 === 0) {
        from = colorBright
        to = colorDark
      } else {
        from = colorDark
        to = colorBright
      }

      light.transition(from, to, 600, 26, render)
        .then(() => {
          count = count + 1
          if (count < 9) {
            delayAndShutdown()
          } else {
            callback()
          }
      })
    }, 100)
  }

  // set degree action here, data.degree
  light.transition({ r: 120, g: 120, b: 120 }, { r: 255, g: 255, b: 255 }, 600, 15, render)
    .then(() => {
      light.requestAnimationFrame(() => {
        delayAndShutdown()
      }, 600)
  })

  if (data && data.withAwaken) {
    light.playAwake()
  }
}
