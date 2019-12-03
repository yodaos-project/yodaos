'use strict'

var logger = require('logger')('products-light')

module.exports = function awake (light, data, callback) {
  var count = 0
  var colorBright = { r: 255, g: 255, b: 255 }
  var colorDark = { r: 50, g: 50, b: 50 }
  var from = colorBright
  var to = colorDark

  logger.log('------awake start -------')
  function delayAndShutdown () {
      if (count % 2 === 0) {
        from = colorBright
        to = colorDark
      } else {
        from = colorDark
        to = colorBright
      }

    light.transition(from, to, 600, 26, (r, g, b) => {
        light.fill(r, g, b)
        light.render()
    }).then(() => {
          count = count + 1
          if (count < 9) {
        light.requestAnimationFrame(() => {
            delayAndShutdown()
        }, 100)
          } else {
            callback()
          }
      })
  }

  if (data.degree !== undefined) {
    // set degree action here, data.degree
    light.requestAnimationFrame(delayAndShutdown, 600)
  } else {
    light.fill(255, 255, 255)
      light.render()
      light.requestAnimationFrame(() => {
      light.clear()
        light.render()
      }, 6000)
  }
}
