'use strict'

var path = require('path')
var childProcess = require('child_process')

var Application = require('@yodaos/application').Application
var AudioFocus = require('@yodaos/application').AudioFocus
var SpeechSynthesis = require('@yodaos/speech-synthesis').SpeechSynthesis
var Storage = require('@yodaos/storage')

var _ = require('@yoda/util')._
var safeParse = require('@yoda/util').json.safeParse
var ota = require('@yoda/ota')
var system = require('@yoda/system')
var property = require('@yoda/property')
var MediaPlayer = require('@yoda/multimedia').MediaPlayer
var logger = require('logger')('ota-app')

var constant = require('./constant')

var INFO_ITEM_KEY = 'run-info'

/**
 *
 * @param {YodaRT.Activity} activity
 */
module.exports = function (api) {
  var speechSynthesis = new SpeechSynthesis(api)
  var localStorage = new Storage(path.join('/data/AppData', api.appId))

  function saveOtaInfo (info) {
    localStorage.setItem(INFO_ITEM_KEY, JSON.stringify(info))
  }

  function removeOtaInfo () {
    localStorage.removeItem(INFO_ITEM_KEY)
  }

  function getOtaInfo () {
    var val = localStorage.getItem(INFO_ITEM_KEY)
    if (!val) {
      return
    }
    return safeParse(val)
  }

  function speakAsync (text) {
    var focus = new AudioFocus(AudioFocus.Type.TRANSIENT, api.audioFocus)
    focus.onGain = () =>
      speechSynthesis.speak(text)
        .on('cancel', () => focus.abandon())
        .on('error', () => focus.abandon())
        .on('end', () => focus.abandon())
    focus.onLoss = () =>
      speechSynthesis.cancel()
    focus.request()
  }

  function playExAsync (url) {
    return new Promise((resolve, reject) => {
      var focus = new AudioFocus(AudioFocus.Type.TRANSIENT_EXCLUSIVE, api.audioFocus)
      var player = new MediaPlayer()
      player.prepare(url)
      player.on('error', reject)
      player.on('cancel', resolve)
      player.on('playbackcomplete', resolve)
      focus.onGain = () =>
        player.resume()
      focus.onLoss = () =>
        player.stop()
      focus.request()
    })
  }

  function runWithInfo (info) {
    logger.info('running ota with info', info)
    saveOtaInfo(info)
    var downloadInfo = _.pick(info, 'imageUrl', 'imageSize', 'version', 'integrity')
    childProcess.spawn(process.argv[0], [
      '/usr/yoda/services/otad/index.js',
      '--fetcher', `echo '${JSON.stringify(downloadInfo)}' #`,
      '--integrity', "bash -c 'printf \"$1  $0\" | md5sum -c'",
      '--notify', `bash ${__dirname}/script/notify`
    ], { detached: true, stdio: 'ignore' })
  }

  /**
   *
   */
  function onFirstBootAfterUpgrade () {
    var info = getOtaInfo()
    if (info == null || typeof info.version !== 'string' || info.version.length <= 0) {
      return
    }
    var localVersion = property.get('ro.build.version.release')
    if (_.startsWith(localVersion, info.version)) {
      logger.info('system version matched precedent ota version, cleanup ota.')
      removeOtaInfo()
      ota.resetOta(function onReset (err) {
        if (err) {
          logger.error('Unexpected error on reset ota', err.stack)
        }
      })
    }
  }

  function startUpgrade (version, imagePath) {
    var info = getOtaInfo()
    if (info == null) {
      logger.error(`no precedent ota request saved, discarding upgrade request.`)
      return
    }
    if (info.version !== version) {
      logger.error(`version mismatch, discarding upgrade request.`)
      return
    }
    logger.info(`using ota image ${imagePath} with info ${JSON.stringify(info)}`)
    var ret = system.prepareOta(imagePath)
    if (ret !== 0) {
      logger.error(`OTA prepared with status code ${ret}, terminating.`)
      return info.silent && speakAsync(constant.OTA_PREPARATION_FAILED)
    }
    var future = Promise.resolve()
    if (!(info.flags & constant.flags.silent)) {
      var media = '/opt/media/ota_start_update.ogg'
      future = playExAsync(media)
        .catch(err => {
          logger.error('Unexpected error on announcing start update', err.stack)
        })
    }
    return future
      .then(() => system.reboot('ota'))
  }

  var app = Application({
    url: function url (url) {
      logger.info(`received url ${url.pathname}?${require('querystring').stringify(url.query)}`)
      switch (url.pathname) {
        case '/run_with_info':
          runWithInfo(url.query)
          break
        case '/upgrade':
          startUpgrade(url.query.version, url.query.image_path)
          break
      }
    },
    broadcast: function broadcast (channel) {
      logger.info(`received broadcast(${channel})`)
      switch (channel) {
        case 'yodaos.on-system-booted':
          onFirstBootAfterUpgrade()
          break
      }
    }
  }, api)

  return app
}
