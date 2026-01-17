const MESSAGE_PARAMS_REGEX = /({([a-zA-Z0-9_]+)(%.*?)?})/g;

enum LocalizableParamType {
  STRING,
  INTEGER,
}
interface LocalizableParam {
  placeholder: string;
  name: string;
  type: LocalizableParamType;
}

function parseMessageParams(message: string) {
  const params: LocalizableParam[] = [];
  const seenParams = new Set<string>();

  const matches = message.matchAll(MESSAGE_PARAMS_REGEX);
  for (const match of matches) {
    if (match.length == 0) {
      continue;
    }
    const placeholder = match[1];
    const name = match[2];

    // Handles the same key being used more than once in the format
    if (seenParams.has(name)) {
      continue;
    }
    seenParams.add(name);

    const format = match[3];
    let type: LocalizableParamType;
    if (!format || format === '' || format === '%s') {
      type = LocalizableParamType.STRING;
    } else if (format === '%d') {
      type = LocalizableParamType.INTEGER;
    } else {
      throw new Error(`Unsupported format type: ${format}`);
    }
    const param: LocalizableParam = {
      placeholder,
      name,
      type,
    };
    params.push(param);
  }
  return params;
}

export function messageToFormat(baseEnglishMessage: string, localizedMessage: string) {
  const baseEnglishParams = parseMessageParams(baseEnglishMessage);
  const localizedMessageParams = parseMessageParams(localizedMessage);
  if (baseEnglishParams.length !== localizedMessageParams.length) {
    throw new Error(
      `Number of placeholders in English string ${baseEnglishMessage} doesn't match the number of placeholders in localized string ${localizedMessage}`,
    );
  }

  let i = 0;
  for (const param of baseEnglishParams) {
    let regexp = RegExp(param.placeholder, 'g');
    localizedMessage = localizedMessage.replace(regexp, `$${i}`);
    i += 1;
  }
  return localizedMessage;
}

export function compareKeys(l: string, r: string) {
  return l > r ? 1 : -1;
}

export function formatForIOSLocalizableStrings(string: string) {
  let result = string.replace(/"/g, '\\"').replace(/\\u0020/g, ' '); // Existing hack to allow spaces to be added at the beginning or end of the string
  return result;
}

// HACK: some translate strings JSON files in private_profile include a limited set of XML
//       entities.
//
// Doing this hack to ensure we're good before branch promo
export function hack_privateProfile_replaceXMLEntities(input: string) {
  const result = input
    .replace(/\u0026nbsp;/g, '\u00A0')
    .replace(/\u0026ordm;/g, 'º')
    .replace(/\u0026deg;/g, '°');
  return result;
}

export async function escapeForAndroidXML(string: string, lang: string = 'en') {
  const segmenter = new Intl.Segmenter(lang, { granularity: 'grapheme' });
  const segmentContainers = segmenter.segment(string);

  let result = '';
  for (const segmentContainer of segmentContainers) {
    // A detailed explanation of this regex that is attempting to find just emoji and characters
    // that aren't valid in XML
    // [<>&'] - specific characters that cause problems in XML
    // | - logical or
    // Emoji-matching pattern (see https://github.com/tc39/proposal-regexp-unicode-property-escapes#matching-emoji)
    const regex = /[<>&']|(\p{Emoji_Modifier_Base}\p{Emoji_Modifier}?|\p{Emoji_Presentation}|\p{Emoji}\uFE0F)/gu;
    if (!segmentContainer.segment.match(regex)) {
      result += segmentContainer.segment;
      continue;
    }

    let codepoints = '';
    for (const element of segmentContainer.segment) {
      codepoints += _escapeForAndroidXML(element);
    }
    result += codepoints;
  }
  return result;
}

function _escapeForAndroidXML(c: string) {
  switch (c) {
    case '<':
      return '&lt;';
    case '>':
      return '&gt;';
    case '&':
      return '&amp;';
    case "'":
      // TODO: IMO should replace with &apos, but export_strings.py does _this_ currently
      return "\\'";
    // TODO: IMO should replace quotes too, but export_strings.py does not do this currently
    // case '"':
    //   return '&quot;';
    default:
      return '&#' + c.codePointAt(0) + ';';
  }
}
