import { IUnparsedLocalizableStringResolver } from './IUnparsedLocalizableStringResolver';
import { getLocalizedString } from '../Strings';

export class ExternalUnparsedLocalizableStringResolver implements IUnparsedLocalizableStringResolver {
  constructor(readonly module: string) {}

  resolveUnparsedLocalizableString(key: string): string | undefined {
    return getLocalizedString(this.module, key);
  }
}
