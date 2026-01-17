import { IProviderSource } from './IProviderSource';
import { ProviderKey } from './ProviderKey';

/**
 * An implementation of IProviderSource that holds a provider and a value,
 * and contains a pointer to its parent. When resolving values,
 * the ProviderSource goes up the chain until it finds a match
 * for the given provider.
 */
class ProviderSourceWithValue<TValue> implements IProviderSource {
  constructor(readonly parent: IProviderSource, readonly key: ProviderKey<TValue>, readonly value: TValue) {}

  getValue<T>(key: ProviderKey<T>): T {
    if (this.key.keyName === key.keyName) {
      return this.value as unknown as T;
    }

    return this.parent.getValue(key);
  }

  hasValue<T>(key: ProviderKey<T>): boolean {
    if (this.key.keyName === key.keyName) {
      return true;
    }

    return this.parent.hasValue(key);
  }

  withValue<T>(key: ProviderKey<T>, value: T): IProviderSource {
    return new ProviderSourceWithValue(this, key, value);
  }
}

/**
 * A IProviderSource implementation that acts as the header for the ProviderSource
 * list. It can create new sources with associated values through the withValue() API.
 */
export class ProviderSourceHead implements IProviderSource {
  constructor() {}

  getValue<T>(key: ProviderKey<T>): T {
    throw new Error(`Could not resolve Value for Provider Key '${key.keyName}'`);
  }

  hasValue<T>(key: ProviderKey<T>): boolean {
    return false;
  }

  withValue<T>(key: ProviderKey<T>, value: T): IProviderSource {
    return new ProviderSourceWithValue(this, key, value);
  }
}

export class ProviderSourceList implements IProviderSource {
  constructor(readonly providerSources: IProviderSource[]) {}

  getValue<T>(key: ProviderKey<T>): T {
    for (const providerSource of this.providerSources) {
      if (providerSource.hasValue(key)) {
        return providerSource.getValue(key);
      }
    }
    throw new Error(`Could not resolve Value for Provider Key '${key.keyName}'`);
  }

  hasValue<T>(key: ProviderKey<T>): boolean {
    for (const providerSource of this.providerSources) {
      if (providerSource.hasValue(key)) {
        return true;
      }
    }
    return false;
  }

  withValue<T>(key: ProviderKey<T>, value: T): IProviderSource {
    return new ProviderSourceWithValue(this, key, value);
  }
}
