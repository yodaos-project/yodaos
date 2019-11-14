'use strict'

var Application = require('@yodaos/application').Application
var SpeechSynthesis = require('@yodaos/speech-synthesis').SpeechSynthesis
var AudioFocus = require('@yodaos/application').AudioFocus

var logger = require('logger')('@battery')
var util = require('util')
var path = require('path')
var prop = require('@yoda/property')
var battery = require('@yoda/battery')
var MediaPlayer = require('@yoda/multimedia').MediaPlayer

var Const = require('./constant.json')

var PROP_KEY = 'rokid.battery10.times'
var TEMPERATURE_LIGHT_RES = 'system://temperatureBattery.js'
var constant = Const.constant
var resourcePath = Const.resource

function parseTime (time) {
  if (time < 0) {
    return {
      hour: 0,
      minute: 0
    }
  }
  var h = Math.floor(time / 60)
  var m = time % 60
  if (h < 0) {
    h = 0
  }
  return {
    hour: h,
    minute: m
  }
}

function playMediaAsync (url) {
  return new Promise((resolve, reject) => {
    if (!path.isAbsolute(url)) {
      url = path.resolve(__dirname, url)
    }
    logger.log('notify media:', url)
    var player = new MediaPlayer()
    player.start(url)
    player.on('playbackcomplete', resolve)
    player.on('error', reject)
  })
}

function lowerPower (percent, isPlaying, focus) {
  var url
  if (percent === 10) {
    url = resourcePath.lowPower10
  } else if (percent === 20) {
    if (isPlaying && isPlaying === 'true') {
      url = resourcePath.lowPower20Play
    } else {
      url = resourcePath.lowPower20Idle
    }
  }
  playMediaAsync(url)
    .then(() => focus && focus.abandon())
}

function temperatureAbnormal (isHighTemperature) {
  logger.warn('temperatureAbnormal:', isHighTemperature)
  var url
  if (isHighTemperature) {
    url = resourcePath.temperature50
  } else {
    url = resourcePath.temperature0
  }
  playMediaAsync(url)
}

module.exports = function (api) {
  var speechSynthesis = new SpeechSynthesis(api)
  function speakAsync (text) {
    logger.log('speak:', text)
    return new Promise((resolve, reject) => {
      speechSynthesis.speak(text)
        .on('error', reject)
        .on('cancel', resolve)
        .on('end', resolve)
    })
  }

  function withoutBattery (focus) {
    speakAndAbandon(constant.batteryNotEnabled, focus)
  }

  function speakAndAbandon (text, focus) {
    return speakAsync(text).then(
      () => focus.abandon(),
      err => {
        logger.error('unexpected error on speech synthesis', err.stack)
        focus.abandon()
      })
  }

  function powerStatusChange (isOnline, isPlaying, focus) {
    logger.log('powerStatusChanged ', isOnline, isPlaying)
    var sound = isOnline ? resourcePath.powerPlug : resourcePath.powerPull
    playMediaAsync(sound)
      .then(() => {
        if (isOnline || isPlaying === false) {
          logger.log('powerStatusChange will exit', isOnline, isPlaying)
          focus && focus.abandon()
          return
        }
        return battery.getBatteryInfo()
          .then(data => {
            logger.log('queryBatteryStatus end:', JSON.stringify(data))
            var percent = data.batLevel
            var text
            var timeObj
            if (percent >= 20) {
              timeObj = parseTime(data.batTimetoEmpty)
              text = util.format(constant.batteryDisconnect20, data.batLevel, timeObj.hour, timeObj.minute)
            } else {
              if (data.batSleepTimetoEmpty < 0 || data.batTimetoEmpty < 0) {
                logger.log('powerStatusChange invalid battery info will exit:')
                // api.exit()
                return
              }
              var times = prop.get(PROP_KEY, 'persist')
              times = times ? parseInt(times) : 0
              logger.log('powerStatusChanged percent < 20:', times, typeof (times))
              if (times < 3) {
                timeObj = parseTime(data.batSleepTimetoEmpty)
                text = util.format(constant.batteryDisconnect19third, timeObj.hour, timeObj.minute)
                logger.log('powerStatusChanged low than 20:', times, timeObj.hour, timeObj.minute)
                prop.set(PROP_KEY, times + 1, 'persist')
              } else {
                timeObj = parseTime(data.batTimetoEmpty)
                text = util.format(constant.batteryDisconnect19, timeObj.hour, timeObj.minute)
                logger.log('powerStatusChanged low than 20:', times, timeObj.hour, timeObj.minute)
              }
            }
            speakAndAbandon(text, focus)
          })
      })
  }

  var temperatureTimeId
  function pollingCheckTemperature () {
    if (temperatureTimeId) {
      logger.warn('temperature check timer is started')
      return
    }
    temperatureTimeId = setInterval(function () {
      // check temperature if not safe will notifyLight again or safe will cancel timer
      logger.warn('temperature timer callback will check again')
      battery.getBatteryInfo()
        .then(data => {
          if (data.batTemp >= 55 || data.batTemp <= 0) {
            api.effect.play(TEMPERATURE_LIGHT_RES)
          } else {
            clearInterval(temperatureTimeId)
          }
        })
    }, 30 * 1000)
  }

  function temperatureAbnormalLight (isHighTemperature) {
    logger.warn('temperatureAbnormalLight:', isHighTemperature)
    api.effect.play(TEMPERATURE_LIGHT_RES)
  }

  function batteryUseTime (focus) {
    battery.getBatteryInfo()
      .then(data => {
        if (data.batSupported === false) {
          withoutBattery(focus)
          return
        }
        if (data.batChargingOnline) {
          speakAndAbandon(constant.timeToEmptyConnect, focus)
        } else {
          var useTime = data.batTimetoEmpty
          var timeObj = parseTime(useTime)
          var text = util.format(constant.timeToEmptyDisconnect, data.batLevel || 100, timeObj.hour, timeObj.minute)
          speakAndAbandon(text, focus)
        }
      })
  }

  function batteryLevel (focus) {
    battery.getBatteryInfo()
      .then((data) => {
        if (!data) {
          logger.warn('queryBatteryStatus failed')
          return
        }
        if (data.batSupported === false) {
          withoutBattery(focus)
          return
        }
        if (data.batLevel && data.batLevel === 100) {
          speakAndAbandon(constant.batteryLevelFull, focus)
        } else {
          speakAndAbandon(util.format(constant.batteryLevel, data.batLevel || 0), focus)
        }
      })
  }

  function batteryCharging (focus) {
    battery.getBatteryInfo()
      .then(batteryState => {
        logger.log('intent batteryCharging:', JSON.stringify(batteryState))
        if (batteryState.batSupported === false) {
          withoutBattery(focus)
          return
        }
        var text
        if (batteryState.batChargingOnline && batteryState.batTimetoFull !== -1) {
          if (batteryState.batLevel && batteryState.batLevel === 100) {
            text = constant.timeToFull100
          } else {
            var timeToFull = batteryState.batTimetoFull || 0
            var timeObj = parseTime(timeToFull)
            text = util.format(constant.timeToFull, timeObj.hour, timeObj.minute)
          }
        } else {
          if (batteryState.batChargingOnline && batteryState.batTimetoFull === -1) {
            text = util.format(constant.timeToFullPowerLow, batteryState.batLevel || 0)
          } else {
            text = util.format(constant.timeToFullDisconnect, batteryState.batLevel || 0)
          }
        }
        speakAndAbandon(text, focus)
      })
  }

  function feedback (pathname, isPlaying, focus) {
    switch (pathname) {
      case '/battery_usetime':
        batteryUseTime(focus)
        return true
      case '/battery_charging':
        batteryCharging(focus)
        return true
      case '/battery_level':
        batteryLevel(focus)
        return true
      case '/low_power_10':
        lowerPower(10, isPlaying, focus)
        return isPlaying
      case '/power_off':
        powerStatusChange(false, isPlaying, focus)
        return isPlaying
      case '/temperature_55':
        temperatureAbnormal(true, focus)
        return true
      case '/temperature_0':
        temperatureAbnormal(false, focus)
        return true
    }
  }

  function silentFeedback (pathname, isPlaying) {
    switch (pathname) {
      case '/low_power_20':
        lowerPower(20, isPlaying)
        return true
      case '/power_on':
        powerStatusChange(true)
        return true
      case '/power_off':
        powerStatusChange(false, isPlaying)
        return true
      case '/temperature_light_55':
        temperatureAbnormalLight(true)
        pollingCheckTemperature()
        return true
      case '/temperature_light_0':
        temperatureAbnormalLight(false)
        pollingCheckTemperature()
        return true
    }
  }

  var app = Application({
    url: function url (url) {
      logger.info('received url', url.href)
      var ran = false
      var focus = new AudioFocus(AudioFocus.Type.TRANSIENT, api.audioFocus)
      ran = feedback(url.pathname, url.query.is_play === 'true', focus)
      if (!ran) {
        silentFeedback(url.pathname, url.query.is_play === 'true')
        return
      }
      focus.request()
    }
  }, api)

  return app
}
