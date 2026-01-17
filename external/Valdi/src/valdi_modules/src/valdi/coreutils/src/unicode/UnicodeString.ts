import { codepointsToStr, strToCodepoints } from './UnicodeNative';

export const enum UnicodeFlags {
  NONE = 0,
  CONTROL = 1 << 0,
  FORMAT = 1 << 1,
  UNASSIGNED = 1 << 2,
  PRIVATE_USE = 1 << 3,
  SURROGATE = 1 << 4,
  LOWERCASE_LETTER = 1 << 5,
  MODIFIER_LETTER = 1 << 6,
  OTHER_LETTER = 1 << 7,
  TITLECASE_LETTER = 1 << 8,
  UPPERCASE_LETTER = 1 << 9,
  SPACING_MARK = 1 << 10,
  ENCLOSING_MARK = 1 << 11,
  NON_SPACING_MARK = 1 << 12,
  DECIMAL_NUMBER = 1 << 13,
  LETTER_NUMBER = 1 << 14,
  OTHER_NUMBER = 1 << 15,
  CONNECT_PUNCTUATION = 1 << 16,
  DASH_PUNCTUATION = 1 << 17,
  CLOSE_PUNCTUATION = 1 << 18,
  FINAL_PUNCTUATION = 1 << 19,
  INITIAL_PUNCTUATION = 1 << 20,
  OTHER_PUNCTUATION = 1 << 21,
  OPEN_PUNCTUATION = 1 << 22,
  CURRENCY_SYMBOL = 1 << 23,
  MODIFIER_SYMOL = 1 << 24,
  MATH_SYMBOL = 1 << 25,
  OTHER_SYMBOL = 1 << 26,
  LINE_SEPARATOR = 1 << 27,
  PARAGRAPH_SEPARATOR = 1 << 28,
  SPACE_SEPARATOR = 1 << 29,
  EMOJI = 1 << 30,
  FROM_GRAPHEME_CLUSTER = 1 << 31,
}

export const enum UnicodeFlagsMask {
  LETTER = UnicodeFlags.LOWERCASE_LETTER |
    UnicodeFlags.MODIFIER_LETTER |
    UnicodeFlags.OTHER_LETTER |
    UnicodeFlags.TITLECASE_LETTER |
    UnicodeFlags.UPPERCASE_LETTER,

  NUMBER = UnicodeFlags.DECIMAL_NUMBER | UnicodeFlags.LETTER_NUMBER | UnicodeFlags.OTHER_NUMBER,

  PUNCTUATION = UnicodeFlags.CONNECT_PUNCTUATION |
    UnicodeFlags.DASH_PUNCTUATION |
    UnicodeFlags.CLOSE_PUNCTUATION |
    UnicodeFlags.FINAL_PUNCTUATION |
    UnicodeFlags.INITIAL_PUNCTUATION |
    UnicodeFlags.OTHER_PUNCTUATION |
    UnicodeFlags.OPEN_PUNCTUATION,

  SEPARATOR = UnicodeFlags.LINE_SEPARATOR | UnicodeFlags.PARAGRAPH_SEPARATOR | UnicodeFlags.SPACE_SEPARATOR,

  SYMBOL = UnicodeFlags.CURRENCY_SYMBOL |
    UnicodeFlags.MODIFIER_SYMOL |
    UnicodeFlags.MATH_SYMBOL |
    UnicodeFlags.OTHER_SYMBOL,

  /**
   * Mask to check whether the unicode point will result in a visible glyph
   */
  VISIBLE_GLYPH = LETTER | NUMBER | PUNCTUATION | SEPARATOR | SYMBOL,
}

export const enum UnicodeStringOptions {
  NONE = 0,

  /**
   * Whether unicode points should be combined if possible.
   */
  NORMALIZE = 1 << 0,

  /**
   * Whether to disable categorization of each character.
   * This will speed up processing of the string but will
   * be missing all the flags information.
   */
  NO_CATEGORIZATION = 1 << 1,
}

abstract class UnicodeStringBase {
  abstract readonly length: number;

  abstract getCodepointAt(index: number): number;
  abstract getFlagsAt(index: number): UnicodeFlags;

  isEmojiAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlags.EMOJI) != 0;
  }

  isLetterAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlagsMask.LETTER) != 0;
  }

  isNumber(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlagsMask.NUMBER) != 0;
  }

  isPunctuationAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlagsMask.PUNCTUATION) != 0;
  }

  isSeparatorAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlagsMask.SEPARATOR) != 0;
  }

  isSurrogateAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlags.SURROGATE) != 0;
  }

  isModifierSymbolAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlags.MODIFIER_SYMOL) != 0;
  }

  isFormatAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlags.FORMAT) != 0;
  }

  isControlAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlags.CONTROL) != 0;
  }

  isNewlineAt(index: number): boolean {
    return this.getCodepointAt(index) == 0x000a;
  }

  isWhitespaceAt(index: number): boolean {
    return this.isSeparatorAt(index) || this.isNewlineAt(index);
  }

  isFromGraphemeClusterAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlags.FROM_GRAPHEME_CLUSTER) != 0;
  }

  isVisibleAt(index: number): boolean {
    return (this.getFlagsAt(index) & UnicodeFlagsMask.VISIBLE_GLYPH) != 0;
  }

  /**
   * Counts the number of grapheme clusters within the string
   */
  countGraphemeClusters(): number {
    const length = this.length;
    let count = 0;
    for (let i = 0; i < length; i++) {
      if (!this.isFromGraphemeClusterAt(i)) {
        count++;
      }
    }

    return count;
  }
}

let spaceConstant: UnicodeString | undefined;

/**
 * UnicodeString is a wrapper around a UTF32 string, with methods to help
 * querying about unicode information for each character.
 */
export class UnicodeString extends UnicodeStringBase {
  /**
   * Returns the number of UTF32 codepoints in the string.
   */
  readonly length: number;

  /**
   * Returns a JS string representing the UnicodeString.
   */
  readonly str: string;

  private buffer: Uint32Array;

  private constructor(str: string, buffer: Uint32Array) {
    super();
    this.str = str;
    this.length = buffer.length / 2;
    this.buffer = buffer;
  }

  getCodepointAt(index: number): number {
    return this.buffer[index];
  }

  getFlagsAt(index: number): UnicodeFlags {
    return this.buffer[this.length + index];
  }

  toMutable(): MutableUnicodeString {
    const codepoints: number[] = [];
    const flags: number[] = [];
    for (let i = 0, length = this.length; i < length; i++) {
      codepoints.push(this.getCodepointAt(i));
      flags.push(this.getFlagsAt(i));
    }
    return new MutableUnicodeString(codepoints, flags);
  }

  /**
   * Create a UnicodeString from a JS string.
   * if "includeCategorization" is false, flags for each symbol won't be resolved.
   */
  static fromString(str: string, options: UnicodeStringOptions = UnicodeStringOptions.NONE): UnicodeString {
    const normalize = (options & UnicodeStringOptions.NORMALIZE) != 0;
    const disableCategorization = (options & UnicodeStringOptions.NO_CATEGORIZATION) != 0;
    const uni = strToCodepoints(str, normalize, disableCategorization);
    return new UnicodeString(str, new Uint32Array(uni));
  }

  /**
   * Create a UnicodeString from an array of UTF32 codepoints.
   * if "includeCategorization" is false, flags for each symbol won't be resolved.
   */
  static fromCodepoints(
    codepoints: number[],
    options: UnicodeStringOptions = UnicodeStringOptions.NONE,
  ): UnicodeString {
    const normalize = (options & UnicodeStringOptions.NORMALIZE) != 0;
    const disableCategorization = (options & UnicodeStringOptions.NO_CATEGORIZATION) != 0;
    const uni = codepointsToStr(codepoints, normalize, disableCategorization);
    return new UnicodeString(uni.str, new Uint32Array(uni.buffer));
  }

  /**
   * Returns a UnicodeString constant containing a single space
   */
  static space(): UnicodeString {
    if (!spaceConstant) {
      spaceConstant = UnicodeString.fromString(' ');
    }
    return spaceConstant;
  }
}

/**
 * MutableUnicodeString is a class that helps with manipulating UnicodeStrings.
 */
export class MutableUnicodeString extends UnicodeStringBase {
  get length(): number {
    return this.codepoints.length;
  }

  private codepoints: number[];
  private flags: number[];

  constructor(codepoints: number[], flags: number[]) {
    super();
    this.codepoints = codepoints;
    this.flags = flags;
  }

  getCodepointAt(index: number): number {
    return this.codepoints[index];
  }

  getFlagsAt(index: number): UnicodeFlags {
    return this.flags[index];
  }

  trim(): MutableUnicodeString {
    return this.trimRight().trimLeft();
  }

  trimLeft(): MutableUnicodeString {
    while (this.length > 0 && this.isWhitespaceAt(0)) {
      this.codepoints.shift();
      this.flags.shift();
    }

    return this;
  }

  trimRight(): MutableUnicodeString {
    for (;;) {
      const endIndex = this.length - 1;
      if (endIndex < 0 || !this.isWhitespaceAt(endIndex)) {
        break;
      }

      this.codepoints.pop();
      this.flags.pop();
    }
    return this;
  }

  removeAt(index: number): MutableUnicodeString {
    return this.removeCount(index, 1);
  }

  /**
   * Remove the codepoints from the given inclusive startIndex to the given exclusive endIndex.
   */
  removeRange(startIndex: number, endIndex: number): MutableUnicodeString {
    if (endIndex < startIndex) {
      throw new Error(`Invalid range ${startIndex} to ${endIndex}`);
    }
    return this.removeCount(startIndex, endIndex - startIndex);
  }

  /**
   * Remove a number of codepoints from the given startIndex
   */
  removeCount(startIndex: number, count: number): MutableUnicodeString {
    this.codepoints.splice(startIndex, count);
    this.flags.splice(startIndex, count);
    return this;
  }

  /**
   * Insert the given codepoint and flags at the given index
   */
  insertAt(index: number, codepoint: number, flags: UnicodeFlags): MutableUnicodeString {
    this.codepoints.splice(index, 0, codepoint);
    this.flags.splice(index, 0, flags);

    return this;
  }

  /**
   * Only keep the grapheme clusters for which the include function returns true.
   * The include function will be called for each grapheme cluster, providing the startIndex
   * of the graphem cluster within the UnicodeString and its exclusive endIndex.
   */
  filterGraphemeClusters(
    include: (self: MutableUnicodeString, startIndex: number, endIndex: number) => boolean,
  ): MutableUnicodeString {
    return this.replaceGraphemeClusters(
      (self, startIndex, endIndex) => !include(self, startIndex, endIndex),
      undefined,
    );
  }

  /**
   *
   * Replace the grapheme clusters for which include returns true by the given replacement UnicodeString.
   * The include function will be called for each grapheme cluster, providing the startIndex
   * of the graphem cluster within the UnicodeString and its exclusive endIndex.
   * If "replacement" is undefined, it will be replaced by empty string.
   * @returns
   */
  replaceGraphemeClusters(
    include: (self: MutableUnicodeString, startIndex: number, endIndex: number) => boolean,
    replacement: UnicodeString | undefined,
  ): MutableUnicodeString {
    let graphemeClusterStart = -1;
    let current = 0;
    while (current < this.length) {
      if (!this.isFromGraphemeClusterAt(current)) {
        // New grapheme cluster
        if (graphemeClusterStart >= 0) {
          const graphemeClusterEnd = current;
          if (include(this, graphemeClusterStart, graphemeClusterEnd)) {
            const toRemoveCount = graphemeClusterEnd - graphemeClusterStart;
            this.removeCount(graphemeClusterStart, toRemoveCount);
            current -= toRemoveCount;

            if (replacement) {
              for (let i = 0; i < replacement.length; i++) {
                this.insertAt(current, replacement.getCodepointAt(i), replacement.getFlagsAt(i));
                current++;
              }
            }
          }
        }
        graphemeClusterStart = current;
      }

      current++;
    }

    if (graphemeClusterStart >= 0) {
      if (include(this, graphemeClusterStart, current)) {
        const toRemoveCount = current - graphemeClusterStart;
        this.removeCount(graphemeClusterStart, toRemoveCount);

        if (replacement) {
          for (let i = 0; i < replacement.length; i++) {
            this.insertAt(current, replacement.getCodepointAt(i), replacement.getFlagsAt(i));
            current++;
          }
        }
      }
    }

    return this;
  }

  /**
   * Split the UnicodeString for each unicode point on which the predicate function
   * returns true.
   */
  splitBy(predicate: (self: MutableUnicodeString, index: number) => boolean): MutableUnicodeString[] {
    const output: MutableUnicodeString[] = [];

    let currentCodepoints: number[] = [];
    let currentFlags: number[] = [];

    for (let i = 0; i < this.length; i++) {
      if (predicate(this, i)) {
        output.push(new MutableUnicodeString(currentCodepoints, currentFlags));
        currentCodepoints = [];
        currentFlags = [];
      } else {
        if (!currentCodepoints) {
          currentCodepoints = [];
        }
        if (!currentFlags) {
          currentFlags = [];
        }
        currentCodepoints.push(this.getCodepointAt(i));
        currentFlags.push(this.getFlagsAt(i));
      }
    }

    output.push(new MutableUnicodeString(currentCodepoints, currentFlags));

    return output;
  }

  /**
   * Split the UnicodeString by whitespaces, which includes newlines and separators
   */
  splitByWhitespace(): MutableUnicodeString[] {
    return this.splitBy((self, index) => self.isWhitespaceAt(index));
  }

  copy(): MutableUnicodeString {
    return new MutableUnicodeString([...this.codepoints], [...this.flags]);
  }

  toUnicodeString(): UnicodeString {
    return UnicodeString.fromCodepoints(this.codepoints);
  }

  toString(): string {
    return codepointsToStr(this.codepoints, false, true).str;
  }
}
