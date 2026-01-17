import { type ChildProcessWithoutNullStreams, spawn } from 'child_process';

export interface SubCommand {
  process: ChildProcessWithoutNullStreams;
  /**
   * Resolvess when the command exits
   */
  wait: () => Promise<number>;
}

export const run = (
  command: string,
  args: string[] = [],
  { pipeOutput = false, cwd }: { pipeOutput?: boolean; cwd?: string } = {},
): SubCommand => {
  const child = spawn(command, args, { detached: true, stdio: 'pipe', cwd });

  if (pipeOutput) {
    child.stderr.pipe(process.stderr);
    child.stdout.pipe(process.stdout);
  }

  const complete = new Promise<number>((resolve, reject) => {
    child.on('close', exitCode => {
      if (exitCode !== null && exitCode !== 0) {
        reject(new Error(command + ' ' + args.join(' ') + ' failed with code ' + exitCode.toString()));
        return;
      }
      resolve(exitCode ?? 0);
    });
  });

  return {
    process: child,
    wait: () => complete,
  };
};
