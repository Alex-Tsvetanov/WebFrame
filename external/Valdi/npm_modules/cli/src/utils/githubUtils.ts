import { runCliCommand } from './cliUtils';

export async function resolveLatestReleaseRef(repoUrl: string): Promise<string> {
  const result = await runCliCommand(`git ls-remote --tags --refs ${repoUrl}`, undefined, true);
  return result.stdout.trim().split(/\s+/)[1]!;
}
