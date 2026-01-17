function makeRangeCheck(fromChar: string, toChar: string): (charCode: number) => boolean {
  const from = fromChar.charCodeAt(0);
  const to = toChar.charCodeAt(0);

  return (charNode) => charNode >= from && charNode <= to;
}

const isUppercaseLetter = makeRangeCheck('A', 'Z');
const isDigit = makeRangeCheck('a', 'z');
const isLowercaseLetter = makeRangeCheck('a', 'z');

export class Identifier {
  private components: string[] = [];

  constructor(str: string) {
    let current = '';

    const flush = () => {
      if (current) {
        this.components.push(current);
        current = '';
      }
    };

    for (let i = 0; i < str.length; i++) {
      const code = str.charCodeAt(i);

      if (isUppercaseLetter(code)) {
        flush();
        current += str.charAt(i).toLowerCase();
      } else if (isDigit(code) || isLowercaseLetter(code)) {
        current += str.charAt(i);
      } else {
        flush();
      }
    }

    flush();
  }

  toCamelCase(): string {
    if (this.components.length < 2) {
      return this.components.join('');
    }

    return (
      this.components[0] +
      this.components
        .slice(1)
        .map((c) => c.charAt(0).toUpperCase() + c.substring(1))
        .join('')
    );
  }

  toPascalCase(): string {
    return this.components.map((c) => c.charAt(0).toUpperCase() + c.substring(1)).join('');
  }

  toSnakeCase(): string {
    return this.components.join('_');
  }
}
