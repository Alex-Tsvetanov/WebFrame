import type { Argv } from 'yargs';

export const command = 'lint <command>';
export const describe = 'Checks and formats the code';
export const builder = (yargs: Argv) => {
  return yargs
    .commandDir('lint_commands', {
      extensions: ['js', 'ts'],
    })
    .demandCommand(1, 'Use "check" or "format" to execute prettier')
    .recommendCommands()
    .wrap(yargs.terminalWidth())
    .help();
};
export const handler = () => {};
