import { ProviderKey } from './ProviderKey';
import { resolveProviderSource } from './resolveProviderSource';

export function resolveProviderValue<TValue>(key: ProviderKey<TValue>): TValue {
  const providerSource = resolveProviderSource();
  if (!providerSource) {
    throw new Error(
      'No ProviderSource found in the tree. Please render a Provider component within this component tree',
    );
  }
  return providerSource.getValue(key);
}
