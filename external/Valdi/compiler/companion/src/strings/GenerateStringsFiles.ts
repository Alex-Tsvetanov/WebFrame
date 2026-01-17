import { readFile } from 'fs/promises';
import {
  compareKeys,
  escapeForAndroidXML,
  formatForIOSLocalizableStrings,
  hack_privateProfile_replaceXMLEntities,
  messageToFormat,
} from './utils';

interface StringsJSONRecord {
  defaultMessage: string;
  description?: string;
  example?: string;
}

interface GenerateStringsJSONResult {
  [key: string]: StringsJSONRecord;
}

export enum LocalizationServiceDirectiveParsingMode {
  Ignore,
  Require,
}

type _iOSKey = string;
type _AndroidKey = string;

interface LocalizableKeyPath {
  moduleName: string;
  key: string;
}

///////////////////////////////////////////////////////////////////////////////
//
// Parsing existing strings.yaml
//
///////////////////////////////////////////////////////////////////////////////

export async function parseStringsJSONAtPath(
  moduleName: string,
  path: string,
  l10nsvcDirectiveParsingMode: LocalizationServiceDirectiveParsingMode,
) {
  const fileContents = await readFile(path);
  const stringsJSON = parseStringsJSON(moduleName, fileContents.toString(), l10nsvcDirectiveParsingMode);
  return stringsJSON;
}

export function parseStringsJSON(
  moduleName: string,
  fileContent: string,
  l10nsvcDirectiveMode: LocalizationServiceDirectiveParsingMode,
) {
  const parsed = JSON.parse(fileContent);
  // Snap-specific: a "l10nsvc": "sourcefile" key/value pair is used as an annotation required by the Snap Localization Service:
  // We ignore it when processing the strings.json file
  const keyToIgnore = 'l10nsvc';
  if (parsed.hasOwnProperty(keyToIgnore)) {
    delete parsed[keyToIgnore];
  } else {
    switch (l10nsvcDirectiveMode) {
      case LocalizationServiceDirectiveParsingMode.Ignore:
        break;
      case LocalizationServiceDirectiveParsingMode.Require:
        throw new Error(
          `Expected l10nsvc directive not found in strings.json for ${moduleName}.\n\n ⚠️ ⚠️ ⚠️\nPlease update ${moduleName}'s strings-en.json to include a "l10nsvc": "sourcefile" key/value pair.\n ⚠️ ⚠️ ⚠️\n`,
        );
    }
  }

  const ordered = sortStringsJSON(parsed);
  return ordered;
}

function sortStringsJSON(stringsJSON: GenerateStringsJSONResult): GenerateStringsJSONResult {
  const ordered = Object.keys(stringsJSON)
    .sort((l, r) => {
      return compareKeys(l, r);
    })
    .reduce((obj, key) => {
      obj[key] = stringsJSON[key];
      return obj;
    }, {} as GenerateStringsJSONResult);
  return ordered;
}

///////////////////////////////////////////////////////////////////////////////
//
// Generating Android strings.xml
//
///////////////////////////////////////////////////////////////////////////////

export async function generateAndroidStringsXMLFromStringsJSON(
  moduleName: string,
  baseEnglishStringsJSON: GenerateStringsJSONResult | undefined,
  inputStringsJSON: GenerateStringsJSONResult,
  stringsJSONLocale: string,
): Promise<string> {
  if (!baseEnglishStringsJSON && stringsJSONLocale !== 'en') {
    throw new Error('Must provide base English strings JSON when generating Android strings for a translation');
  }

  const indent = `    `;

  const header = `<?xml version="1.0" encoding="UTF-8"?><resources>`;

  const prelude = `${header}
${indent}<!-- VALDI GENERATED STRINGS, see https://github.com/Snapchat/Valdi/blob/main/docs/docs/advanced-localization.md -->`;
  // TODO: ^ IMO we should add the Valdi module name to this comment (and maybe a link to the Valdi module in the client repo)
  //         but the export_strings.py does not do this currently

  const postlude = `</resources>\n`;

  let encounteredKeyMap = new Map<_AndroidKey, LocalizableKeyPath>();
  let contents: [string, string][] = [];
  for (const key in inputStringsJSON) {
    const record = inputStringsJSON[key];
    let comment = '';
    if (record.description) {
      // TODO: comment should include the example and should be sanitized
      const cleanedComment = record.description;
      comment = `<!-- ${cleanedComment} -->\n`;
    }
    // TODO: should emit the comment too, but export_strings.py currently doesn't

    // Note: we are _intentionally_ not using record.androidKey here. That is the current behaviour of export_strings.py.
    //       With this migration we can remove all usages of android_key.
    const jsKey = `${moduleName}_${key}`;
    const cleanedKey = jsKey;

    guardAgainstEmittingDuplicateKeys({ moduleName, key }, cleanedKey, encounteredKeyMap);

    let baseEnglishRecord = baseEnglishStringsJSON && baseEnglishStringsJSON[key];
    if (!baseEnglishRecord) {
      if (stringsJSONLocale !== 'en') {
        // Base (English) strings JSON doesn't have the same key as what we're trying to export
        // from the translation.
        //
        // Skipping.
        continue;
      }
      baseEnglishRecord = record;
    }

    const format = messageToFormat(baseEnglishRecord.defaultMessage, record.defaultMessage);
    // HACK: some translated strings JSON files in private_profile include XML entities...
    const hackedFormat = hack_privateProfile_replaceXMLEntities(format);
    const cleanedFormat = await escapeForAndroidXML(hackedFormat, stringsJSONLocale);

    let formatted = '';
    // HACK: this reproduces an existing bug in export_strings.py where we look at the resulting format
    //       I'm not currently sure _when_ we should actually specify formatted="false", so just reproducing quirk-for-quirk
    if (cleanedFormat.indexOf('%') >= 0) {
      formatted = ' formatted="false"';
    }

    const line = `<string name="${cleanedKey}"${formatted}>${cleanedFormat}</string>`;
    contents.push([cleanedKey, line]);
  }

  const sortedContents = contents
    .sort(([lkey], [rkey]) => {
      return compareKeys(lkey, rkey);
    })
    .map(([key, line]) => {
      return line;
    });

  const prettyContents = sortedContents.map((line) => `${indent}${line}`).join('\n');
  return `${prelude}
${prettyContents}
${postlude}`;
}

///////////////////////////////////////////////////////////////////////////////
//
// Generating iOS Localizable.strings
//
///////////////////////////////////////////////////////////////////////////////

export function generateIOSLocalizableStringsFromStringsJSON(
  moduleName: string,
  baseEnglishStringsJSON: GenerateStringsJSONResult | undefined,
  inputStringsJSON: GenerateStringsJSONResult,
  stringsJSONLocale: string,
): string {
  if (!baseEnglishStringsJSON && stringsJSONLocale !== 'en') {
    throw new Error('Must provide base English strings JSON when generating iOS strings for a translation');
  }

  const prelude = `
// BEGIN VALDI AUTOGENERATED see https://github.com/Snapchat/Valdi/blob/main/docs/docs/advanced-localization.md`;
  // TODO: ^ l10nscv comment should be on the next line, but the export_strings.py does not do this currently
  //
  // TODO: ^ IMO we should add the Valdi module name to this comment (and maybe a link to the Valdi module in the client repo)
  //         but the export_strings.py does not do this currently

  const postlude = ``;
  // TODO: ^ // END VALDI AUTOGENERATED, but the export_strings.py does not do this currently

  let encounteredKeyMap = new Map<_iOSKey, LocalizableKeyPath>();
  let contents: [string, string][] = [];
  for (const key in inputStringsJSON) {
    const record = inputStringsJSON[key];
    let comment = '';
    if (record.description) {
      // TODO: comment should include the example and should be sanitized
      const cleanedComment = record.description;
      comment = `/* ${cleanedComment} */`;
    }
    // TODO: should emit the comment too, but the export_strings.py does not do this currently

    guardAgainstEmittingDuplicateKeys({ moduleName, key }, key, encounteredKeyMap);

    let baseEnglishRecord = baseEnglishStringsJSON && baseEnglishStringsJSON[key];
    if (!baseEnglishRecord) {
      if (stringsJSONLocale !== 'en') {
        // Base (English) strings JSON doesn't have the same key as what we're trying to export
        // from the translation.
        //
        // Skipping.
        continue;
      }
      baseEnglishRecord = record;
    }

    const format = messageToFormat(baseEnglishRecord.defaultMessage, record.defaultMessage);
    // HACK: some translated strings JSON files in private_profile include XML entities...
    const hackedFormat = hack_privateProfile_replaceXMLEntities(format);
    const cleanedFormat = formatForIOSLocalizableStrings(hackedFormat);

    // TODO: should emit the comment too, but the export_strings.py does not do this currently
    const line = `"${key}" = "${cleanedFormat}";`;
    contents.push([key, line]);
  }

  const sortedContents = contents
    .sort(([lkey], [rkey]) => {
      return compareKeys(lkey, rkey);
    })
    .map(([key, line]) => {
      return line;
    });

  const prettyContents = sortedContents.join('\n');
  return `${prelude}
${prettyContents}
${postlude}`;
}

///////////////////////////////////////////////////////////////////////////////
//
// Internal utilities
//
///////////////////////////////////////////////////////////////////////////////

function guardAgainstEmittingDuplicateKeys(
  current: LocalizableKeyPath,
  generatedPlatformKey: _iOSKey | _AndroidKey,
  encounteredKeyMap: Map<_iOSKey | _AndroidKey, LocalizableKeyPath>,
) {
  const previouslyEncountered = encounteredKeyMap.get(generatedPlatformKey);
  if (previouslyEncountered) {
    throw new Error(
      `Platform string key ${generatedPlatformKey} already encountered. Currently processing module: ${current.moduleName} key: ${current.key}. Previously encountered in module: ${previouslyEncountered.moduleName} key: ${previouslyEncountered.key}`,
    );
  }
  encounteredKeyMap.set(generatedPlatformKey, current);
}
