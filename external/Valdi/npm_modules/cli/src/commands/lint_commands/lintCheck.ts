import type { Argv } from 'yargs';
import { ANSI_COLORS } from '../../core/constants';
import type { ArgumentsResolver } from '../../utils/ArgumentsResolver';
import { makeCommandHandler } from '../../utils/errorUtils';
import type { FileLintStatus } from '../../utils/lintUtils';
import {
  checkAndFormatFiles,
  getPrettierConfig,
  maybeSetupEslint,
  maybeSetupPrettier,
  printStatus,
} from '../../utils/lintUtils';
import { wrapInColor } from '../../utils/logUtils';

interface CommandParameters {
  files: string[] | undefined;
  showSkipped: boolean;
}

async function lintCheck(argv: ArgumentsResolver<CommandParameters>) {
  const configPath = await maybeSetupPrettier();
  const prettierConfig = await getPrettierConfig(configPath);
  await maybeSetupEslint();

  console.log(`Using config at: ${wrapInColor(configPath, ANSI_COLORS.GREEN_COLOR)}\n`);

  const filePaths = argv.getArgument('files');

  if (filePaths === undefined || prettierConfig === null) {
    return;
  }

  const results: FileLintStatus[] = await checkAndFormatFiles(filePaths, prettierConfig);

  // Output list of files and their status
  printStatus(results, argv.getArgument('showSkipped'));

  // TODO (yhuang6): Implement the following
  // Default checks changed files based on git
  // Default checks the current directory otherwise
}

export const command = 'check [files..]';
export const describe = 'Checks the file formatting and lint rules';
export const builder = (yargs: Argv<CommandParameters>) => {
  yargs.option('show-skipped', {
    describe: 'Shows files skipped during linting (e.g. no format parsers)',
    type: 'boolean',
    alias: 's',
    default: false,
  });
};
export const handler = makeCommandHandler(lintCheck);
