export type ProviderKeyName = string;

export interface ProviderKey<TValue> {
  keyName: ProviderKeyName;
  brand?: TValue;
}

/**
 * Create a ProviderKey with the given name.
 */
export function createProviderKey<TValue>(name: ProviderKeyName): ProviderKey<TValue> {
  return {
    keyName: name,
  };
}
