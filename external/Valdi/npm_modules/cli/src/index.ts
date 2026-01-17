#!/usr/bin/env node

import yargs from 'yargs';
import { hideBin } from 'yargs/helpers';
import { version } from '../package.json';

function main() {
  void yargs(hideBin(process.argv))
    .completion()
    .commandDir('commands', {
      extensions: ['js', 'ts'],
    })
    .demandCommand(1, 'Need at least one command to execute')
    .recommendCommands()
    .strict()
    .version('version', 'Show version number', version)
    .scriptName('valdi')
    .wrap(yargs.terminalWidth())
    .help().argv;
}

main();
