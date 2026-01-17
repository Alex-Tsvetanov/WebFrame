// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import * as faker from './lib/faker';

export class ReproducibleGenerator {
  private faker: any;

  /**
   * Need to pass a seed to ensure the reproducibility of the generated values
   */
  constructor(seed: number) {
    this.faker = new faker.constructor({ locales: faker.locales });
    this.faker.seed(seed);
  }
  private setLocale(locale?: string) {
    if (!locale) {
      this.faker.locale = this.faker.random.locale();
    } else {
      this.faker.locale = locale;
    }
  }

  nextUniqueId(): string {
    return this.faker.random.uuid();
  }

  nextInternetName(locale?: string): string {
    this.setLocale(locale);
    return this.faker.internet.userName();
  }
  nextFreeformName(locale?: string): string {
    this.setLocale(locale);
    return this.faker.name.findName();
  }
  nextFreeformWords(locale?: string): string {
    this.setLocale(locale);
    return this.faker.name.findName(); // Unfortunately, only names have proper freeform locale support for now
  }
  nextPhoneNumber(locale?: string): string {
    this.setLocale(locale);
    return this.faker.phone.phoneNumber();
  }
  nextAlphaNumericString(size: number): string {
    return this.faker.random.alphaNumeric(size);
  }

  /**
   * returns true/false
   */
  nextBoolean() {
    return this.faker.random.boolean();
  }
  /**
   * returns a number in range [0, 1] included
   */
  nextFloat() {
    return this.faker.random.number({
      min: 0,
      max: 1,
      precision: 0.001,
    });
  }
  /**
   * returns a number in range [0, 1.000.000.000] included
   */
  nextInt() {
    return this.faker.random.number({
      min: 0,
      max: 1000000000,
      precision: 1,
    });
  }
  /**
   * returns a number in range [start, end] included
   */
  nextRange(start: number, end: number) {
    return this.faker.random.number(end - start) + start;
  }
  /**
   * returns a timestamp in ms between 2010 and 2020
   */
  nextPastTimestampMs(): number {
    // We need to have a stable date range, otherwise generated value will change over time
    return this.faker.date.between('2010-01-01', '2020-01-01').getTime();
  }
  /**
   * returns a dummy friendmoji
   */
  nextFriendmoji(): string {
    const options = ['🔥', '🥰', '💚', '❤️', '🎥'];
    const size = this.nextRange(0, 7);
    let friendmoji = '';
    for (let i = 0; i < size; i++) {
      friendmoji += this.faker.random.arrayElement(options);
    }
    return friendmoji;
  }

  nextLocale(): string {
    return this.faker.random.locale();
  }
  availableLocales() {
    return Object.keys(this.faker.locales);
  }
}
