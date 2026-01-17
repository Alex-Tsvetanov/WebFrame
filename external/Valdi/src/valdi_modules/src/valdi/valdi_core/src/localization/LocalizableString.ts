import { formatNumber } from 'valdi_core/src/utils/NumberUtils';

export type LocalizableStringPart = string | number;

export enum LocalizableStringParameterType {
  STRING = 0,
  NUMBER = 1,
}

export class LocalizableString {
  /**
   * The original, not parsed string
   */

  readonly original: string;

  /**
   * The deconstructed parts of the localizable strings.
   * When the value is a number, it represents the argument index that should be
   * used to build the output string. When the value is a string, it represents
   * a constant part.
   */
  readonly parts: readonly LocalizableStringPart[];

  constructor(original: string, parts: readonly LocalizableStringPart[]) {
    this.original = original;
    this.parts = parts;
  }

  format(types: LocalizableStringParameterType[], args: any[]): string {
    const out: string[] = [];

    for (const part of this.parts) {
      if (typeof part === 'number') {
        if (part < 0 || part >= types.length || part >= args.length) {
          throw new Error(`Out of bounds argument ${part} in localizable string ${this.original}`);
        }

        const type = types[part];
        const arg = args[part];

        if (arg === undefined) {
          out.push('<undefined>');
        } else if (arg === null) {
          out.push('<null>');
        } else {
          switch (type) {
            case LocalizableStringParameterType.NUMBER:
              out.push(formatNumber(arg));
              break;
            case LocalizableStringParameterType.STRING:
              out.push(arg.toString());
              break;
          }
        }
      } else {
        out.push(part);
      }
    }

    return out.join('');
  }

  static parse(input: string): LocalizableString {
    const tokens = input.split(/\$(\d+)/g);
    const parts: LocalizableStringPart[] = [];

    for (const token of tokens) {
      const arg = parseInt(token, 10);
      // Checks if the entire token is an integer.
      if (isNaN(arg) || token !== arg.toString()) {
        parts.push(token);
      } else {
        parts.push(arg);
      }
    }

    return new LocalizableString(input, parts);
  }
}
