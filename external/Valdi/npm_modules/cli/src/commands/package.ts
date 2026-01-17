import * as fs from 'fs';
import path from 'path';
import type { Argv } from 'yargs';
import type { PLATFORM } from '../core/constants';
import { ALL_ARCHITECTURES, ANSI_COLORS } from '../core/constants';
import { CliError } from '../core/errors';
import type { ArgumentsResolver } from '../utils/ArgumentsResolver';
import { BazelClient } from '../utils/BazelClient';
import type { SharedCommandParameters } from '../utils/applicationUtils';
import {
  applicationExtensionForPlatform,
  getApplicationTargetTagForPlatform,
  getOutputFilePath,
  makeArgsBuilder,
  resolveBazelBuildArgs,
  selectBazelTarget,
} from '../utils/applicationUtils';
import { logReproduceThisCommandIfNeeded, makeCommandHandler } from '../utils/errorUtils';
import { wrapInColor } from '../utils/logUtils';

interface CommandParameters extends SharedCommandParameters {
  application: string | undefined;
  target_output_path: string | undefined;
  output_path: string;
}

async function valdiPackage(argv: ArgumentsResolver<CommandParameters>): Promise<void> {
  const bazel = new BazelClient();
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  const platform = argv.getArgument('platform') as PLATFORM;
  const bazelArgs = resolveBazelBuildArgs(
    platform,
    argv.getArgument('build_config'),
    ALL_ARCHITECTURES,
    /* forDevice */ true,
    argv.getArgument('enable_runtime_logs'),
    argv.getArgument('enable_runtime_traces'),
    argv.getArgument('bazel_args'),
  );

  const resolvedOutputPath = path.resolve(argv.getArgument('output_path'));
  const expectedExtension = applicationExtensionForPlatform(platform);
  if (expectedExtension.length > 0 && !resolvedOutputPath.endsWith(expectedExtension)) {
    throw new CliError(`Output path for platform '${platform}' must have ${expectedExtension} file extension`);
  }

  if (fs.existsSync(resolvedOutputPath)) {
    fs.rmSync(resolvedOutputPath);
  }
  const directoryPath = path.dirname(resolvedOutputPath);
  if (!fs.existsSync(directoryPath)) {
    fs.mkdirSync(directoryPath, { recursive: true });
  }

  const application = await argv.getArgumentOrResolve('application', () => {
    console.log('No application specified querying available targets...');
    return selectBazelTarget(
      bazel,
      getApplicationTargetTagForPlatform(platform),
      'valdi_application',
      'Please select an application target to package:',
    );
  });
  console.log('Resolving output paths...');

  const bazelOutputFilePath = await argv.getArgumentOrResolve('target_output_path', () => {
    console.log('Resolving output paths...');
    return getOutputFilePath(bazel, expectedExtension, application, bazelArgs);
  });
  console.log(`Building: ${wrapInColor(application, ANSI_COLORS.GREEN_COLOR)}`);

  await bazel.buildTarget(application, bazelArgs);

  console.log(`Copying built target to ${resolvedOutputPath}...`);
  await fs.promises.copyFile(bazelOutputFilePath, resolvedOutputPath);

  logReproduceThisCommandIfNeeded(argv);
  console.log('All done!');
}

export const command = 'package <platform>';
export const describe = 'Build and package a Valdi application';
export const builder = makeArgsBuilder((yargs: Argv<CommandParameters>) => {
  yargs
    .option('application', {
      describe: 'Name of the application to build',
      type: 'string',
      requiresArg: true,
    })
    .option('target_output_path', {
      describe: 'The output path for the Bazel target',
      type: 'string',
      requiresArg: true,
    })
    .option('output_path', {
      describe: 'The path where to store the built application package',
      type: 'string',
      requiresArg: true,
    })
    .demandOption('output_path');
});
export const handler = makeCommandHandler(valdiPackage);
