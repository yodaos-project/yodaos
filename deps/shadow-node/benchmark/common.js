'use strict';

var child_process = require('child_process');

exports.createBenchmark = function(fn, configs, options) {
  return new Benchmark(fn, configs, options);
};

function Benchmark(fn, configs, options) {
  // Use the file name as the name of the benchmark
  this.name = require.main.filename.slice(__dirname.length + 1);
  // Parse job-specific configuration from the command line arguments
  var parsed_args = this._parseArgs(process.argv.slice(2), configs);
  this.options = parsed_args.cli;
  this.extra_options = parsed_args.extra;
  // The configuration list as a queue of jobs
  this.queue = this._queue(this.options);
  // The configuration of the current job, head of the queue
  this.config = this.queue[0];
  // Execution arguments i.e. flags used to run the jobs
  this.flags = [];
  if (options && options.flags) {
    this.flags = this.flags.concat(options.flags);
  }
  // Holds process.hrtime value
  this._time = [0, 0];
  // Used to make sure a benchmark only start a timer once
  this._started = false;

  process.nextTick(() => fn(this.config));
  // process.nextTick(this._run.bind(this));
}

Benchmark.prototype._parseArgs = function(argv, configs) {
  var cliOptions = {};
  var extraOptions = {};
  var validArgRE = /^(.+?)=([\s\S]*)$/;

  // Parse configuration arguments
  argv.forEach((arg) => {
    var match = arg.match(validArgRE);
    if (!match) {
      console.error(`bad argument: ${arg}`);
      return process.exit(1);
    }
    var config = match[1];

    if (configs[config]) {
      // Infer the type from the config object and parse accordingly
      var isNumber = typeof configs[config][0] === 'number';
      var value = isNumber ? +match[2] : match[2];
      if (!cliOptions[config])
        cliOptions[config] = [];
      cliOptions[config].push(value);
    } else {
      extraOptions[config] = match[2];
    }
  });
  return {
    cli: Object.assign({}, configs, cliOptions),
    extra: extraOptions
  };
};

Benchmark.prototype._queue = function(options) {
  var queue = [];
  var keys = Object.keys(options);

  // Perform a depth-first walk though all options to generate a
  // configuration list that contains all combinations.
  function recursive(keyIndex, prevConfig) {
    var key = keys[keyIndex];
    var values = options[key];
    var type = typeof values[0];

    values.forEach((value) => {
      if (typeof value !== 'number' && typeof value !== 'string')
        throw new TypeError(`configuration "${key}" had type ${typeof value}`);

      if (typeof value !== type) {
        // This is a requirement for being able to consistently and predictably
        // parse CLI provided configuration values.
        throw new TypeError(`configuration "${key}" has mixed types`);
      }

      var _config = {};
      _config[key] = value;
      var currConfig = Object.assign(_config, prevConfig);

      if (keyIndex + 1 < keys.length) {
        recursive(keyIndex + 1, currConfig);
      } else {
        queue.push(currConfig);
      }
    });
  }

  if (keys.length > 0) {
    recursive(0, {});
  } else {
    queue.push({});
  }

  return queue;
};

Benchmark.prototype._run = function() {
  var self = this;
  // If forked, report to the parent.
  if (process.send) {
    process.send({
      type: 'config',
      name: this.name,
      queueLength: this.queue.length
    });
  }

  (function recursive(queueIndex) {
    var config = self.queue[queueIndex];

    // set NODE_RUN_BENCHMARK_FN to indicate that the child shouldn't construct
    // a configuration queue, but just execute the benchmark function.
    var childEnv = Object.assign({}, process.env);
    childEnv.NODE_RUN_BENCHMARK_FN = '';

    // Create configuration arguments
    var childArgs = [];
    Object.keys(config).forEach((key) => {
      childArgs.push(`${key}=${config[key]}`);
    });
    Object.keys(self.extra_options).forEach((key) => {
      childArgs.push(`${key}=${self.extra_options[key]}`);
    });

    var child = child_process.fork(require.main.filename, childArgs, {
      env: childEnv,
      execArgv: self.flags.concat(process.execArgv)
    });
    child.on('message', sendResult);
    child.on('close', function(code) {
      if (code) {
        process.exit(code);
        return;
      }

      if (queueIndex + 1 < self.queue.length) {
        recursive(queueIndex + 1);
      }
    });
  })(0);
};

Benchmark.prototype.start = function() {
  if (this._started) {
    throw new Error('Called start more than once in a single benchmark');
  }
  this._started = true;
  this._time = process.hrtime();
};

Benchmark.prototype.end = function(operations) {
  // get elapsed time now and do error checking later for accuracy.
  var elapsed = process.hrtime(this._time);

  if (!this._started) {
    throw new Error('called end without start');
  }
  if (typeof operations !== 'number') {
    throw new Error('called end() without specifying operation count');
  }
  if (!process.env.NODEJS_BENCHMARK_ZERO_ALLOWED && operations <= 0) {
    throw new Error('called end() with operation count <= 0');
  }
  if (elapsed[0] === 0 && elapsed[1] === 0) {
    if (!process.env.NODEJS_BENCHMARK_ZERO_ALLOWED)
      throw new Error('insufficient clock precision for short benchmark');
    // avoid dividing by zero
    elapsed[1] = 1;
  }

  var time = elapsed[0] + elapsed[1] / 1e9;
  var rate = operations / time;
  this.report(rate, elapsed);
};

function formatResult(data) {
  // Construct configuration string, " A=a, B=b, ..."
  var conf = '';
  Object.keys(data.conf).forEach((key) => {
    conf += ` ${key}=${JSON.stringify(data.conf[key])}`;
  });

  var rate = data.rate.toString().split('.');
  rate[0] = rate[0].replace(/(\d)(?=(?:\d\d\d)+(?!\d))/g, '$1,');
  rate = (rate[1] ? rate.join('.') : rate[0]);
  return `${data.name}${conf}: ${rate}`;
}

function sendResult(data) {
  if (process.send) {
    // If forked, report by process send
    process.send(data);
  } else {
    // Otherwise report by stdout
    console.log(formatResult(data));
  }
}
exports.sendResult = sendResult;

Benchmark.prototype.report = function(rate, elapsed) {
  sendResult({
    name: this.name,
    conf: this.config,
    rate: rate,
    time: elapsed[0] + elapsed[1] / 1e9,
    type: 'report'
  });
};
