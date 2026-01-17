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
  files: [string] | undefined;
  showSkipped: boolean;
}

async function lintFormat(argv: ArgumentsResolver<CommandParameters>) {
  const configPath = await maybeSetupPrettier();
  const prettierConfig = await getPrettierConfig(configPath);
  await maybeSetupEslint();

  console.log(`Using config at: ${wrapInColor(configPath, ANSI_COLORS.GREEN_COLOR)}\n`);

  const filePaths = argv.getArgument('files');

  if (filePaths === undefined || prettierConfig === null) {
    return;
  }

  const results: FileLintStatus[] = await checkAndFormatFiles(filePaths, prettierConfig, true);

  // Output list of files and their status
  printStatus(results, argv.getArgument('showSkipped'));
}

export const command = 'format [files..]';
export const describe = 'Apply prettier formatting to files';
export const builder = (yargs: Argv<CommandParameters>) => {
  yargs.option('show-skipped', {
    describe: 'Shows files skipped during linting (e.g. no format parsers)',
    type: 'boolean',
    alias: 's',
    default: false,
  });
};
export const handler = makeCommandHandler(lintFormat);
