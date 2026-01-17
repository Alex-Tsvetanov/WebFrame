import type { Argv } from 'yargs';
import { ANSI_COLORS } from '../core/constants';
import type { ArgumentsResolver } from '../utils/ArgumentsResolver';
import { BazelClient } from '../utils/BazelClient';
import { LoadingIndicator } from '../utils/LoadingIndicator';
import { makeCommandHandler } from '../utils/errorUtils';
import { wrapInColor } from '../utils/logUtils';

interface CommandParameters {
  module: string[] | undefined;
  target: string[] | undefined;
  bazel_args: string | undefined;
}

async function getTestTargetsByModuleNames(client: BazelClient, modules?: string[]): Promise<string[]> {
  return await client.queryTargetsByKindWithFilter('test', modules ?? [], 'pipe');
}

async function valdiTest(argv: ArgumentsResolver<CommandParameters>) {
  const bazelClient = new BazelClient();

  const targets = await argv.getArgumentOrResolve('target', () => {
    return LoadingIndicator.fromTask(getTestTargetsByModuleNames(bazelClient, argv.getArgument('module') ?? []))
      .setText(wrapInColor('Collecting targets for tests...', ANSI_COLORS.YELLOW_COLOR))
      .setSuccessText(wrapInColor('Finished collecting test targets.', ANSI_COLORS.GREEN_COLOR))
      .setFailureText(wrapInColor('Failed to collect targets.', ANSI_COLORS.RED_COLOR))
      .show();
  });

  if (targets === undefined || targets.length === 0) {
    console.log('No test targets found.');
    return;
  }

  console.log('Starting tests for targets:');
  console.log(
    targets
      .map(target => {
        return `  ${wrapInColor(target, ANSI_COLORS.GREEN_COLOR)}`;
      })
      .join('\n'),
  );

  await bazelClient.testTargets(targets, argv.getArgument('bazel_args'));
}

export const command = 'test [--module module_name] [--target target_name]';
export const describe =
  'Runs tests for given module(s) or target(s). Runs all tests if no module or target is specified.';
export const builder = (yargs: Argv<CommandParameters>) => {
  yargs
    .option('module', {
      describe: 'name of the module to test',
      type: 'array',
      requiresArg: true,
    })
    .option('target', {
      describe: 'Bazel target path to test',
      type: 'array',
      requiresArg: true,
    })
    .option('bazel_args', {
      describe: `Additional arguments to pass to bazel build. Use format ${wrapInColor('--bazel_args="<args>"', ANSI_COLORS.YELLOW_COLOR)}.`,
      type: 'string',
      requiresArg: true,
    });
};
export const handler = makeCommandHandler(valdiTest);
