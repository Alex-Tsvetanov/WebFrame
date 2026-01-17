export class Locale {
  readonly language: string;
  readonly region: string | undefined;

  constructor(language: string, region: string | undefined) {
    this.language = language.toLowerCase();
    this.region = region?.toLocaleUpperCase();
  }

  toString(): string {
    if (this.region) {
      return `${this.language}-${this.region}`;
    } else {
      return this.language;
    }
  }

  static parse(input: string): Locale {
    const components = input.split('-');
    return new Locale(components[0], components[1]);
  }
}
