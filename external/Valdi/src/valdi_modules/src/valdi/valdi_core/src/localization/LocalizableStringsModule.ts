import { StringMap } from 'coreutils/src/StringMap';
import { toCamelCase } from '../utils/StringUtils';
import { IUnparsedLocalizableStringResolver } from './IUnparsedLocalizableStringResolver';
import { LocalizableString, LocalizableStringParameterType } from './LocalizableString';

const EXPORT_KEY = Symbol();

export interface LocalizableStringsModuleExports {
  [key: string]: (...formatArgs: any[]) => string;
}

export class LocalizableStringsModule {
  exports: LocalizableStringsModuleExports;

  get stringResolver(): IUnparsedLocalizableStringResolver {
    return this._stringResolver;
  }

  set stringResolver(value: IUnparsedLocalizableStringResolver) {
    if (this._stringResolver !== value) {
      this._stringResolver = value;
      this.cache = {};
    }
  }

  private _stringResolver: IUnparsedLocalizableStringResolver;
  private cache: StringMap<LocalizableString> = {};

  constructor(stringResolver: IUnparsedLocalizableStringResolver) {
    this._stringResolver = stringResolver;
    this.exports = {};
    (this.exports as any)[EXPORT_KEY] = this;
  }

  add(key: string, parameterTypes: LocalizableStringParameterType[] | undefined): void {
    const property = toCamelCase(key);

    if (!parameterTypes) {
      this.exports[property] = (): string => {
        return this.getOrCreateLocalizableString(key).original;
      };
    } else {
      this.exports[property] = (...formatArgs: any[]): string => {
        return this.getOrCreateLocalizableString(key).format(parameterTypes, formatArgs);
      };
    }
  }

  private getOrCreateLocalizableString(key: string): LocalizableString {
    let localizableString = this.cache[key];
    if (!localizableString) {
      const str = this._stringResolver.resolveUnparsedLocalizableString(key) || key;
      localizableString = LocalizableString.parse(str);
      this.cache[key] = localizableString;
    }

    return localizableString;
  }

  static fromExports(exports: LocalizableStringsModuleExports): LocalizableStringsModule {
    const module = (exports as any)[EXPORT_KEY] as LocalizableStringsModule;
    if (!module) {
      throw new Error('Not an exports object');
    }

    return module;
  }
}
