const jasmineRequire = require('./jasmine', true);
const runJasmineOriginBoot = require('./origin_boot', true);
const ConsoleReporter = require('./console_reporter', true);

function Jasmine(showColors, stackFilter, seed, specFilter) {
  const consoleReporter = new ConsoleReporter();
  consoleReporter.setOptions({ print: process.stdout.write, showColors: showColors, stackFilter: stackFilter });

  const jasmine = runJasmineOriginBoot(jasmineRequire);
  const env = jasmine.getEnv({ suppressLoadErrors: true });

  env.configure({
    specFilter: specFilter,
    // Configure random seed if provided
    ...(seed !== undefined && seed !== null ? { random: true, seed: seed } : {}),
  });

  env.addReporter(consoleReporter);

  this.execute = function () {
    env.execute();
  };

  this.addReporter = function (reporter) {
    env.addReporter(reporter);
  };
}

module.exports = Jasmine;
