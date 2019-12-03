'use strict'

var logger = require('logger')('products-light')

module.exports = function setStandby (light, data, callback) {
  var index = 0
  var time = 0
  var action = data.action
  logger.log('------setStandby start -------')

  var circle = function (duration, flag) {
    light.breathing(255, 255, 255, duration, 15, (r, g, b, lastFrame) => {
      light.clear()
      if (!flag) {
        light.pixel(0, r, g, b)
      } else {
        light.pixel(0, 0, 0, 0)
      }
      light.render()
    }).then(() => {
        flag = !flag
        time = flag ? 600 : 1000
        light.requestAnimationFrame(() => {
          circle(time, flag)
      }, 100)
    })
  }
  function render (r, g, b) {
      light.fill(r, g, b)
      light.render()
  }
  if (action === 'reconnect') {
    light.transition({ r: 0, g: 0, b: 0 }, { r: 51, g: 51, b: 51 }, 200, 15, render)
      .then(() => {
        light.requestAnimationFrame(() => {
          light.transition({ r: 51, g: 51, b: 51 }, { r: 255, g: 255, b: 255 }, 3000, 26, render)
            .then(() => {
              circle(1000, true)
          })
        }, 100)
    })
  } else {
    circle(1000, false)
  }

  return {
    shouldResume: true
  }
}
