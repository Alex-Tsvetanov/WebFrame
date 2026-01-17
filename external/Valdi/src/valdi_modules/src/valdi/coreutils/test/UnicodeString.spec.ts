import { UnicodeString, UnicodeStringOptions } from 'coreutils/src/unicode/UnicodeString';
import 'jasmine/src/jasmine';

describe('UnicodeString', () => {
  it('detects letters', () => {
    const str = UnicodeString.fromString('Hello, World!');

    expect(str.length).toEqual(13);
    expect(str.isLetterAt(0)).toBeTrue();
    expect(str.isLetterAt(1)).toBeTrue();
    expect(str.isLetterAt(2)).toBeTrue();
    expect(str.isLetterAt(3)).toBeTrue();
    expect(str.isLetterAt(4)).toBeTrue();
    expect(str.isLetterAt(5)).toBeFalse();
    expect(str.isLetterAt(6)).toBeFalse();
    expect(str.isLetterAt(7)).toBeTrue();
    expect(str.isLetterAt(8)).toBeTrue();
    expect(str.isLetterAt(9)).toBeTrue();
    expect(str.isLetterAt(10)).toBeTrue();
    expect(str.isLetterAt(11)).toBeTrue();
    expect(str.isLetterAt(12)).toBeFalse();
  });

  it('detects numbers', () => {
    const str = UnicodeString.fromString('H3ll0 W0rld');

    expect(str.length).toEqual(11);
    expect(str.isNumber(0)).toBeFalse();
    expect(str.isNumber(1)).toBeTrue();
    expect(str.isNumber(2)).toBeFalse();
    expect(str.isNumber(3)).toBeFalse();
    expect(str.isNumber(4)).toBeTrue();
    expect(str.isNumber(5)).toBeFalse();
    expect(str.isNumber(6)).toBeFalse();
    expect(str.isNumber(7)).toBeTrue();
    expect(str.isNumber(8)).toBeFalse();
    expect(str.isNumber(9)).toBeFalse();
    expect(str.isNumber(10)).toBeFalse();
  });

  it('detects punctuations', () => {
    const str = UnicodeString.fromString('Ok,wow! (great...)');

    expect(str.length).toEqual(18);

    expect(str.isPunctuationAt(0)).toBeFalse();
    expect(str.isPunctuationAt(1)).toBeFalse();
    expect(str.isPunctuationAt(2)).toBeTrue();
    expect(str.isPunctuationAt(3)).toBeFalse();
    expect(str.isPunctuationAt(4)).toBeFalse();
    expect(str.isPunctuationAt(5)).toBeFalse();
    expect(str.isPunctuationAt(6)).toBeTrue();
    expect(str.isPunctuationAt(7)).toBeFalse();
    expect(str.isPunctuationAt(8)).toBeTrue();
    expect(str.isPunctuationAt(9)).toBeFalse();
    expect(str.isPunctuationAt(10)).toBeFalse();
    expect(str.isPunctuationAt(11)).toBeFalse();
    expect(str.isPunctuationAt(12)).toBeFalse();
    expect(str.isPunctuationAt(13)).toBeFalse();
    expect(str.isPunctuationAt(14)).toBeTrue();
    expect(str.isPunctuationAt(15)).toBeTrue();
    expect(str.isPunctuationAt(16)).toBeTrue();
    expect(str.isPunctuationAt(17)).toBeTrue();
  });

  it('detects separators', () => {
    const str = UnicodeString.fromString('Nice boy\n');

    expect(str.length).toEqual(9);

    expect(str.isSeparatorAt(0)).toBeFalse();
    expect(str.isSeparatorAt(1)).toBeFalse();
    expect(str.isSeparatorAt(2)).toBeFalse();
    expect(str.isSeparatorAt(3)).toBeFalse();
    expect(str.isSeparatorAt(4)).toBeTrue();
    expect(str.isSeparatorAt(5)).toBeFalse();
    expect(str.isSeparatorAt(6)).toBeFalse();
    expect(str.isSeparatorAt(7)).toBeFalse();
    expect(str.isSeparatorAt(8)).toBeFalse();
    expect(str.isControlAt(8)).toBeTrue();
    expect(str.isNewlineAt(8)).toBeTrue();
  });

  it('detects emoji', () => {
    const str = UnicodeString.fromString('Wow 😀');

    expect(str.length).toBe(5);

    expect(str.isEmojiAt(0)).toBeFalse();
    expect(str.isEmojiAt(1)).toBeFalse();
    expect(str.isEmojiAt(2)).toBeFalse();
    expect(str.isEmojiAt(3)).toBeFalse();
    expect(str.isEmojiAt(4)).toBeTrue();
  });

  it('can normalize', () => {
    const str = UnicodeString.fromCodepoints([0x41, 0x0301], UnicodeStringOptions.NORMALIZE);
    expect(str.length).toEqual(1);
  });

  it('detects modifiers', () => {
    const str = UnicodeString.fromString('👍🏻👍🏼👍🏽👍🏾👍🏿');

    expect(str.length).toEqual(10);

    expect(str.isEmojiAt(0)).toBeTrue();
    expect(str.isModifierSymbolAt(0)).toBeFalse();
    expect(str.isEmojiAt(1)).toBeTrue();
    expect(str.isModifierSymbolAt(1)).toBeTrue();
    expect(str.isEmojiAt(2)).toBeTrue();
    expect(str.isModifierSymbolAt(2)).toBeFalse();
    expect(str.isEmojiAt(3)).toBeTrue();
    expect(str.isModifierSymbolAt(3)).toBeTrue();
    expect(str.isEmojiAt(4)).toBeTrue();
    expect(str.isModifierSymbolAt(4)).toBeFalse();
    expect(str.isEmojiAt(5)).toBeTrue();
    expect(str.isModifierSymbolAt(5)).toBeTrue();
    expect(str.isEmojiAt(6)).toBeTrue();
    expect(str.isModifierSymbolAt(6)).toBeFalse();
    expect(str.isEmojiAt(7)).toBeTrue();
    expect(str.isModifierSymbolAt(7)).toBeTrue();
    expect(str.isEmojiAt(8)).toBeTrue();
    expect(str.isModifierSymbolAt(8)).toBeFalse();
    expect(str.isEmojiAt(9)).toBeTrue();
    expect(str.isModifierSymbolAt(9)).toBeTrue();
  });

  it('can count grapheme clusters', () => {
    let str = UnicodeString.fromString('👍🏻👍🏼👍🏽👍🏾👍🏿');

    expect(str.countGraphemeClusters()).toBe(5);

    str = UnicodeString.fromString('👨‍❤️‍👨');

    expect(str.countGraphemeClusters()).toBe(1);
    expect(str.length).toBe(6);
  });

  it('can create unicode from codepoints', () => {
    const str = UnicodeString.fromCodepoints([78, 105, 99, 101, 33]);

    expect(str.length).toBe(5);
    expect(str.str).toBe('Nice!');

    expect(str.isLetterAt(0)).toBeTrue();
    expect(str.isLetterAt(1)).toBeTrue();
    expect(str.isLetterAt(2)).toBeTrue();
    expect(str.isLetterAt(3)).toBeTrue();
    expect(str.isPunctuationAt(4)).toBeTrue();
  });
});

describe('MutableUnicodeString', () => {
  it('can trimLeft', () => {
    expect(UnicodeString.fromString('    Hello World    ').toMutable().trimLeft().toString()).toBe('Hello World    ');
  });

  it('can trimRight', () => {
    expect(UnicodeString.fromString('    Hello World    ').toMutable().trimRight().toString()).toBe('    Hello World');
  });

  it('can trim', () => {
    expect(UnicodeString.fromString('    Hello World    ').toMutable().trim().toString()).toBe('Hello World');
  });

  it('can removeAt', () => {
    const str = UnicodeString.fromString('Hello World').toMutable();
    str.removeAt(0);
    expect(str.toString()).toBe('ello World');
    str.removeAt(4);
    expect(str.toString()).toBe('elloWorld');
  });

  it('can removeRange', () => {
    const str = UnicodeString.fromString('Hello World').toMutable();
    str.removeRange(0, 6);

    expect(str.toString()).toBe('World');

    str.removeRange(1, 2);

    expect(str.toString()).toBe('Wrld');
  });

  it('can removeCount', () => {
    const str = UnicodeString.fromString('Hello World').toMutable();
    str.removeCount(6, 5);

    expect(str.toString()).toBe('Hello ');
  });

  it('can filter by grapheme cluster', () => {
    const str = UnicodeString.fromString('Wow 😀, this is great 👍🏻!').toMutable();

    str.filterGraphemeClusters((s, startIndex) => !s.isEmojiAt(startIndex));

    expect(str.toString()).toBe('Wow , this is great !');

    expect(
      UnicodeString.fromString('...\n')
        .toMutable()
        .filterGraphemeClusters((s, startIndex) => !s.isNewlineAt(startIndex))
        .toString(),
    ).toBe('...');
  });

  it('can replace grapheme clusters', () => {
    const replacement = UnicodeString.fromString('<redacted>');
    const str = UnicodeString.fromString('Wow 😀, this is great 👍🏻!').toMutable();

    str.replaceGraphemeClusters((s, startIndex) => s.isEmojiAt(startIndex), replacement);

    expect(str.toString()).toBe('Wow <redacted>, this is great <redacted>!');
  });

  it('can split', () => {
    expect(
      UnicodeString.fromString('Hello\nBonjour\nDzień dobry\n')
        .toMutable()
        .splitBy((s, index) => s.isNewlineAt(index))
        .map(f => f.toString()),
    ).toEqual(['Hello', 'Bonjour', 'Dzień dobry', '']);
  });

  it('can split by whitespace', () => {
    expect(
      UnicodeString.fromString('Hello\nBonjour\nDzień dobry')
        .toMutable()
        .splitByWhitespace()
        .map(f => f.toString()),
    ).toEqual(['Hello', 'Bonjour', 'Dzień', 'dobry']);
  });
});
