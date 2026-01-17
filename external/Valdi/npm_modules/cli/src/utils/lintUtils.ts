/**
 * Common functions shared between the linting commands
 */
import fs from 'fs';
import path from 'path';
import eslint, { ESLint } from 'eslint';
import prettier from 'prettier';
import { ANSI_COLORS, TEMPLATE_BASE_PATHS } from '../core/constants';
import { CliError } from '../core/errors';
import { getUserConfirmation } from './cliUtils';
import { TemplateFile, fileExists, hasExtension, isDirectory } from './fileUtils';
import { wrapInColor } from './logUtils';

export enum RESULT_STATUS {
  SKIPPED = 'skipped',
  DIRECTORY = 'directory',
  NEEDS_FORMAT = 'needs format',
  VALID = 'valid',
  CHANGED = 'changed',
}

export type FileLintStatus = [string, RESULT_STATUS];

// Returns the config path if it exists, otherwise prompts the user to create one
export async function maybeSetupPrettier(): Promise<string> {
  let prettierConfigPath = await getPrettierConfigPath();

  if (prettierConfigPath === null) {
    const initMessage = `No prettier config found. Would you like to create one here? ${wrapInColor(process.cwd(), ANSI_COLORS.GREEN_COLOR)}`;
    const shouldInitPrettier = await getUserConfirmation(initMessage, true);

    if (!shouldInitPrettier) {
      throw new CliError('Prettier config is required, aborting...');
    }

    prettierConfigPath = createPrettierConfig();

    // Need to clear the cache so the newly created config can be resolved
    // in the same execution cycle
    await prettier.clearConfigCache();
  }

  return prettierConfigPath;
}

export async function getPrettierConfigPath(): Promise<string | null> {
  // Starts searching at process.cwd() by default
  // Note that VALIDing in process.cwd() as a parameter will NOT work
  const prettierConfigPath = await prettier.resolveConfigFile();

  return prettierConfigPath;
}

export function createPrettierConfig(): string {
  const prettierTemplateFile = TemplateFile.init(TEMPLATE_BASE_PATHS.PRETTIER_CONFIG);
  return prettierTemplateFile.expandTemplate();
}

// Prompts user to create eslint config files if they don't exist.
export async function maybeSetupEslint() {
  if (!fileExists('.eslintrc.js')) {
    const initMessage = `No eslintrc found. Would you like to create one here? ${wrapInColor(process.cwd(), ANSI_COLORS.GREEN_COLOR)}`;
    const shouldInitEslintrc = await getUserConfirmation(initMessage, true);

    if (!shouldInitEslintrc) {
      throw new CliError('eslintrc config is required, aborting...');
    }

    createEslintConfig();
  }

  if (!fileExists('package.json')) {
    const initMessage = `No package.json found. Would you like to create one here? ${wrapInColor(process.cwd(), ANSI_COLORS.GREEN_COLOR)}`;
    const shouldInitEslintrc = await getUserConfirmation(initMessage, true);

    if (!shouldInitEslintrc) {
      throw new CliError('package.json config is required, aborting...');
    }

    createPackageConfig();

    throw new CliError('New package.json created, run `npm install` before trying again.');
  }
}

export function createEslintConfig(): string {
  const eslintConfigFile = TemplateFile.init(TEMPLATE_BASE_PATHS.ESLINT_CONFIG);
  return eslintConfigFile.expandTemplate();
}

export function createPackageConfig(): string {
  const packageConfigFile = TemplateFile.init(TEMPLATE_BASE_PATHS.ESLINT_PACKAGE_JSON_CONFIG);
  return packageConfigFile.expandTemplate();
}

export async function getPrettierConfig(configPath: string): Promise<prettier.Options | null> {
  return await prettier.resolveConfig(configPath);
}

export async function checkAndFormatFiles(
  filePaths: string[],
  prettierOptions: prettier.Options,
  shouldFormat: boolean = false,
): Promise<FileLintStatus[]> {
  const linter = new eslint.ESLint({fix: true});
  const results: FileLintStatus[] = await Promise.all(
    filePaths.map(async fp => {
      const filePath = path.resolve(fp);

      if (isDirectory(filePath)) {
        return [fp, RESULT_STATUS.DIRECTORY];
      }

      if (hasExtension(filePath, '.json')) {
        return [fp, RESULT_STATUS.SKIPPED];
      }

      const fileContent = fs.readFileSync(filePath, 'utf8');
      const prettierFileInfo = await prettier.getFileInfo(filePath);

      if (prettierFileInfo.inferredParser === null) {
        return [fp, RESULT_STATUS.SKIPPED];
      }

      const lintResults = await linter.lintFiles([filePath]);
      const needsLint = lintResults.length > 0;
      let isLintFormatted = false;

      if (shouldFormat && needsLint) {
        console.log(lintResults[0]?.messages[0]?.message);
        await ESLint.outputFixes(lintResults);
        isLintFormatted = true;
      }

      const needsPrettier = await prettier.check(fileContent, { ...prettierOptions, filepath: filePath });
      let isPrettierFormatted = false;

      if (shouldFormat && !needsPrettier) {
        const formattedContent = await prettier.format(fileContent, { ...prettierOptions, filepath: filePath });
        fs.writeFileSync(filePath, formattedContent, 'utf8');

        isPrettierFormatted = true;

      }

      if (isPrettierFormatted || isLintFormatted) {
        return [fp, RESULT_STATUS.CHANGED];
      }

      const formattedStatus = isPrettierFormatted && isLintFormatted ? RESULT_STATUS.VALID : RESULT_STATUS.NEEDS_FORMAT;
      return [fp, formattedStatus];
    }),
  );

  return results;
}

export function printStatus(lintResults: FileLintStatus[], showSkipped: boolean = false) {
  let textColor = ANSI_COLORS.RESET_COLOR;

  for (const [shortPath, status] of lintResults) {
    switch (status) {
      case RESULT_STATUS.DIRECTORY: {
        continue;
      }
      case RESULT_STATUS.SKIPPED: {
        if (!showSkipped) {
          continue;
        }

        textColor = ANSI_COLORS.GRAY_COLOR;
        break;
      }
      case RESULT_STATUS.VALID: {
        textColor = ANSI_COLORS.GRAY_COLOR;
        break;
      }
      case RESULT_STATUS.NEEDS_FORMAT: {
        textColor = ANSI_COLORS.YELLOW_COLOR;
        break;
      }
      case RESULT_STATUS.CHANGED: {
        textColor = ANSI_COLORS.YELLOW_COLOR;
        break;
      }
    }

    console.log(`${wrapInColor(shortPath, textColor)} - ${status}`);
  }
}
