///////////////////////////////////////////////////////////////////////////////
//
// Command
//
///////////////////////////////////////////////////////////////////////////////

import { fullLocaleMap } from '../../supported-locales/out';
import { mkdir, unlink, writeFile } from 'fs/promises';
import * as path from 'path';
import { ILogger } from '../logger/ILogger';
import { ExportTranslationStringsRequestBody, ExportTranslationStringsResponseBody } from '../protocol';
import { fileExists } from '../utils/fileUtils';
import {
  generateAndroidStringsXMLFromStringsJSON,
  generateIOSLocalizableStringsFromStringsJSON,
  LocalizationServiceDirectiveParsingMode,
  parseStringsJSONAtPath,
} from './GenerateStringsFiles';

export async function exportStringsFiles(
  moduleName: string,
  inputStringsDir: string,
  iosOutputPath?: string,
  androidOutputPath?: string,
) {
  const exists = await fileExists(inputStringsDir);
  if (!exists) {
    throw new Error(`Expected strings directory at ${inputStringsDir}, but it doesn't exist`);
  }

  if (!iosOutputPath && !androidOutputPath) {
    throw new Error('Asked to export strings, but no output paths provided');
  }

  const promises = [];

  let localeMap = fullLocaleMap;
  for (const inputLocale of localeMap.keys()) {
    const outputLocales = localeMap.get(inputLocale);
    if (!outputLocales) {
      throw new Error(`No known output locales for input locale ${inputLocale}`);
    }

    const baseStringsJSONPath = inputJSONPath(inputStringsDir, 'en');

    const inputStringsJSONPath = inputJSONPath(inputStringsDir, inputLocale);
    const outputIOSLocalizableStringsPath =
      iosOutputPath && iosLocalizableStringsPath(iosOutputPath, outputLocales.ios, moduleName);
    const outputAndroidStringsXMLPath =
      androidOutputPath && androidStringsXMLPath(androidOutputPath, outputLocales.android, moduleName);

    const promise = exportStringsFile(
      moduleName,
      inputLocale,
      baseStringsJSONPath,
      inputStringsJSONPath,
      outputIOSLocalizableStringsPath,
      outputAndroidStringsXMLPath,
    ).catch((reason) => {
      throw new Error(`Failed to export strings file for module '${moduleName}': ${reason}`);
    });
    promises.push(promise);
  }

  await Promise.all(promises);
}

async function exportStringsFile(
  moduleName: string,
  inputLocale: string,
  baseStringsJSONPath: string,
  inputStringsJSONPath: string,
  outputIOSLocalizableStringsPath: string | undefined,
  outputAndroidStringsXMLPath: string | undefined,
) {
  const isExportingBaseLocale = inputLocale === 'en';

  // Translation happens in the repo with the valdi_modules sources.

  const exists = await fileExists(inputStringsJSONPath);
  if (!exists) {
    if (isExportingBaseLocale) {
      throw new Error(`Expected base localization file at ${inputStringsJSONPath}, but it doesn't exist`);
    } else {
      // Translation file for that locale doesn't exist
      return;
    }
  }

  const baseEnglishStringsJSON = await parseStringsJSONAtPath(
    moduleName,
    baseStringsJSONPath,
    LocalizationServiceDirectiveParsingMode.Ignore,
  );
  let l10nsvcDirectiveParsingMode = LocalizationServiceDirectiveParsingMode.Ignore;
  if (isExportingBaseLocale) {
    l10nsvcDirectiveParsingMode = LocalizationServiceDirectiveParsingMode.Require;
  }
  const inputStringsJSON = await parseStringsJSONAtPath(moduleName, inputStringsJSONPath, l10nsvcDirectiveParsingMode);
  if (outputAndroidStringsXMLPath) {
    await unlink(outputAndroidStringsXMLPath).catch((e) => undefined); // ignore errors
    const androidOutput = await generateAndroidStringsXMLFromStringsJSON(
      moduleName,
      baseEnglishStringsJSON,
      inputStringsJSON,
      inputLocale,
    );
    if (androidOutput) {
      const dirname = path.dirname(outputAndroidStringsXMLPath);
      await mkdir(dirname, { recursive: true });
      await writeFile(outputAndroidStringsXMLPath, androidOutput);
    }
  }

  if (outputIOSLocalizableStringsPath) {
    await unlink(outputIOSLocalizableStringsPath).catch((e) => undefined); // ignore errors
    const iosOutput = generateIOSLocalizableStringsFromStringsJSON(
      moduleName,
      baseEnglishStringsJSON,
      inputStringsJSON,
      inputLocale,
    );
    if (iosOutput) {
      const dirname = path.dirname(outputIOSLocalizableStringsPath);
      await mkdir(dirname, { recursive: true });
      await writeFile(outputIOSLocalizableStringsPath, iosOutput);
    }
  }
}

function stringsJSONName(locale: string): string {
  return `strings-${locale}.json`;
}

function inputJSONPath(inputStringsDir: string, inputLocale: string) {
  return path.join(inputStringsDir, stringsJSONName(inputLocale));
}

function iosLocalizableStringName(moduleName: string, outputLocale: string) {
  return path.join(`${outputLocale}.lproj`, `valdi_modules_${moduleName}.strings`);
}

function iosLocalizableStringsPath(iosOutputPath: string, outputLocale: string, moduleName: string) {
  return path.join(iosOutputPath, iosLocalizableStringName(moduleName, outputLocale));
}

function androidStringsXMLName(moduleName: string, outputLocale: string) {
  let valuesDirname = `values-${outputLocale}`;
  if (outputLocale === 'en') {
    valuesDirname = 'values';
  }
  return path.join(valuesDirname, `valdi-strings-${moduleName}.xml`);
}

function androidStringsXMLPath(androidOutputPath: string, outputLocale: string, moduleName: string) {
  return path.join(androidOutputPath, androidStringsXMLName(moduleName, outputLocale));
}

export async function exportTranslationStrings(
  request: ExportTranslationStringsRequestBody,
  logger: ILogger | undefined,
): Promise<ExportTranslationStringsResponseBody> {
  const moduleName = request.moduleName;
  if (!(await fileExists(request.baseLocaleStringsPath))) {
    throw new Error(`Expected base localization file at ${request.baseLocaleStringsPath}, but it doesn't exist`);
  }
  if (!(await fileExists(request.inputLocaleStringsPath))) {
    throw new Error(`Expected input localization file at ${request.inputLocaleStringsPath}, but it doesn't exist`);
  }
  if (path.basename(request.baseLocaleStringsPath) !== stringsJSONName('en')) {
    throw new Error(
      `Expected base localization file strings-en.json, but got ${path.basename(request.baseLocaleStringsPath)}`,
    );
  }

  // Find input locale
  const inputLocale = Array.from(fullLocaleMap.keys()).find((locale: string) => {
    return stringsJSONName(locale) === path.basename(request.inputLocaleStringsPath);
  });
  if (!inputLocale) {
    throw new Error(`Could not find input locale for ${request.inputLocaleStringsPath}`);
  }

  const outputLocales = fullLocaleMap.get(inputLocale);
  if (!outputLocales) {
    throw new Error(`No known output locales for input locale ${inputLocale}`);
  }

  const isExportingBaseLocale = inputLocale === 'en';
  logger?.debug?.(
    `Exporting translation strings for module ${moduleName} and locale ${inputLocale} for ${request.platform}`,
  );

  // Translation happens in the repo with the valdi_modules sources.

  const baseLocaleStringsJSON = await parseStringsJSONAtPath(
    moduleName,
    request.baseLocaleStringsPath,
    LocalizationServiceDirectiveParsingMode.Require,
  );
  const l10nsvcDirectiveParsingMode = isExportingBaseLocale
    ? LocalizationServiceDirectiveParsingMode.Require
    : LocalizationServiceDirectiveParsingMode.Ignore;
  const inputLocaleJSON = await parseStringsJSONAtPath(
    moduleName,
    request.inputLocaleStringsPath,
    l10nsvcDirectiveParsingMode,
  );

  if (request.platform === 'android') {
    const androidOutput = await generateAndroidStringsXMLFromStringsJSON(
      moduleName,
      baseLocaleStringsJSON,
      inputLocaleJSON,
      inputLocale,
    );

    return {
      inputLocale: inputLocale,
      platformLocale: outputLocales.android,
      content: androidOutput,
      outputFileName: androidStringsXMLName(moduleName, outputLocales.android),
    };
  } else if (request.platform === 'ios') {
    const iosOutput = generateIOSLocalizableStringsFromStringsJSON(
      moduleName,
      baseLocaleStringsJSON,
      inputLocaleJSON,
      inputLocale,
    );

    return {
      inputLocale: inputLocale,
      platformLocale: outputLocales.ios,
      content: iosOutput,
      outputFileName: iosLocalizableStringName(moduleName, outputLocales.ios),
    };
  } else {
    throw new Error(`Unknown platform ${request.platform}`);
  }
}
