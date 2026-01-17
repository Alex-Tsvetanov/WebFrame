import { ProviderKey } from './ProviderKey';

/**
 * a ProviderSource holds the source of data for the providers.
 */
export interface IProviderSource {
  /**
   * Returns the value for the given provider.
   * Throws if the value cannot be retrieved.
   */
  getValue<T>(key: ProviderKey<T>): T;

  /**
   * Returns whether the ProviderSource can provide the value
   * with the given key
   */
  hasValue<T>(key: ProviderKey<T>): boolean;

  /**
   * Associate the provider with the given value and return a new ProviderSource
   * that can provide the given value when queried through getValue()
   */
  withValue<T>(key: ProviderKey<T>, value: T): IProviderSource;
}
