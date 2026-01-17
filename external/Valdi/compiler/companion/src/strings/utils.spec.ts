import 'ts-jest';
import { escapeForAndroidXML, formatForIOSLocalizableStrings } from './utils';

import { messageToFormat } from './utils';

describe('parsing message params', () => {
  it('works with integer placeholder', () => {
    const message = 'Field number {fieldNumber%d} is required.';
    const format = messageToFormat(message, message);
    expect(format).toBe('Field number $0 is required.');
  });

  it('works with string placeholder without specifier', () => {
    const message = '{field} is required.';
    const format = messageToFormat(message, message);
    expect(format).toBe('$0 is required.');
  });

  it('works with string placeholder with specifier', () => {
    const message = '{field%s} is required.';
    const format = messageToFormat(message, message);
    expect(format).toBe('$0 is required.');
  });

  it('works with multiple placeholders', () => {
    const message = '{field1} and {field2} are required.';
    const format = messageToFormat(message, message);
    expect(format).toBe('$0 and $1 are required.');
  });

  it('works with multiple placeholders with the same name', () => {
    const message =
      'Snap will share the information entered above with {BrandName}. {BrandName} may also be able to infer that you are in the group of users to which the ad was targeted. {BrandName}’s use of this data is subject to their privacy policy.';
    const format = messageToFormat(message, message);
    expect(format).toBe(
      'Snap will share the information entered above with $0. $0 may also be able to infer that you are in the group of users to which the ad was targeted. $0’s use of this data is subject to their privacy policy.',
    );
  });
});

describe('escapeForAndroidXML', () => {
  it('handles less than', async () => {
    const result = await escapeForAndroidXML('string with two < symbols <');
    expect(result).toStrictEqual('string with two &lt; symbols &lt;');
  });

  it('handles greater than', async () => {
    const result = await escapeForAndroidXML('string with two > symbols >');
    expect(result).toStrictEqual('string with two &gt; symbols &gt;');
  });

  it('handles ampersand', async () => {
    const result = await escapeForAndroidXML('string with two & symbols &');
    expect(result).toStrictEqual('string with two &amp; symbols &amp;');
  });

  it('handles single quotes', async () => {
    const result = await escapeForAndroidXML("string with two ' single ' quotes");
    expect(result).toStrictEqual("string with two \\' single \\' quotes");
  });

  it('handles double quotes', async () => {
    const result = await escapeForAndroidXML('string with two " double " quotes');
    expect(result).toStrictEqual('string with two " double " quotes');
  });

  it('handles double quotes', async () => {
    const result = await escapeForAndroidXML('string with two " double " quotes');
    expect(result).toStrictEqual('string with two " double " quotes');
  });

  it('handles unicode', async () => {
    const result = await escapeForAndroidXML('строчка с кириллицей · и · юникодом');
    expect(result).toStrictEqual('строчка с кириллицей · и · юникодом');
  });

  it('handles escaped unicode', async () => {
    const result = await escapeForAndroidXML('Remind on\u0020');
    expect(result).toStrictEqual('Remind on\u0020');
  });
});

describe('emojiToHTMLEntityForAndroidXML', () => {
  it('handles emoji', async () => {
    const result = await escapeForAndroidXML('Sadface 😞', 'en');
    expect(result).toStrictEqual('Sadface &#128542;');
  });

  it('handles emoji then text', async () => {
    const result = await escapeForAndroidXML('Apple 🍏 and some text, and some more text!', 'en');
    expect(result).toStrictEqual('Apple &#127823; and some text, and some more text!');
  });

  it('handles emoji then unicode', async () => {
    const result = await escapeForAndroidXML('Map 🗺️地图', 'zh-Hant');
    expect(result).toStrictEqual('Map &#128506;&#65039;地图');
  });

  it('handles emoji then unicode then text', async () => {
    const result = await escapeForAndroidXML('Red flag 🚩☝️ and some more things and stuff', 'en');
    expect(result).toStrictEqual('Red flag &#128681;&#9757;&#65039; and some more things and stuff');
  });

  it('handles specific case', async () => {
    const result = await escapeForAndroidXML('Add $0 to your Favorites?', 'en');
    expect(result).toStrictEqual('Add $0 to your Favorites?');
  });

  it('handles speak no evil emoji', async () => {
    const result = await escapeForAndroidXML('🙊', 'en');
    expect(result).toStrictEqual('&#128586;');
  });

  describe("doesn't convert south asian scripts", () => {
    it('bn-BD', async () => {
      // ad_format/strings/strings-bn-BD.json
      // ad_info
      const result = await escapeForAndroidXML('বিজ্ঞাপনের তথ্য', 'bn-BD');
      expect(result).toStrictEqual('বিজ্ঞাপনের তথ্য');
    });

    it('ml-IN', async () => {
      // account_challenge/strings/strings-ml-IN.json
      // challenge_page_title
      const result = await escapeForAndroidXML('പാസ്‍വേഡ് പുനഃസജ്ജീകരിക്കുക', 'bn-BD');
      expect(result).toStrictEqual('പാസ്‍വേഡ് പുനഃസജ്ജീകരിക്കുക');
    });
  });

  it('handles flowers', async () => {
    const result = await escapeForAndroidXML('flowers 🌼🌺🌸', 'en');
    expect(result).toStrictEqual('flowers &#127804;&#127802;&#127800;');
  });

  it('handles family emoji', async () => {
    const result = await escapeForAndroidXML('this is a family 👩‍👩‍👧‍👧 with four people', 'en');
    expect(result).toStrictEqual(
      'this is a family &#128105;&#8205;&#128105;&#8205;&#128103;&#8205;&#128103; with four people',
    );
  });

  it('handles emoji with skin tone', async () => {
    const result = await escapeForAndroidXML('person 👩🏾', 'en');
    expect(result).toStrictEqual('person &#128105;&#127998;');
  });

  it('handles emoji with ZWJ and skin tone', async () => {
    const result = await escapeForAndroidXML('people 👩‍👩🏾', 'en');
    expect(result).toStrictEqual('people &#128105;&#8205;&#128105;&#127998;');
  });
});

describe('formatForIOSLocalizableStrings', () => {
  it('handles single quotes', () => {
    const result = formatForIOSLocalizableStrings("string with two ' single ' quotes");
    expect(result).toStrictEqual("string with two ' single ' quotes");
  });

  it('handles double quotes', () => {
    const result = formatForIOSLocalizableStrings('string with two " double " quotes');
    expect(result).toStrictEqual('string with two \\" double \\" quotes');
  });

  it('handles unicode', () => {
    const result = formatForIOSLocalizableStrings('строчка с кириллицей · и · юникодом');
    expect(result).toStrictEqual('строчка с кириллицей · и · юникодом');
  });

  it('handles escaped unicode', () => {
    const result = formatForIOSLocalizableStrings('Remind on\u0020');
    expect(result).toStrictEqual('Remind on ');
  });
});
