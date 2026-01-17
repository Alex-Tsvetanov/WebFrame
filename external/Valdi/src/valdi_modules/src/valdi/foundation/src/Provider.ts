import { Lazy } from 'foundation/src/Lazy';
import { isUndefined } from 'foundation/src/isUndefined';

/**
 * Used to provide lazy dependencies to valdi context
 * @ExportModel
 */
export interface Provider<T> {
  get(): T;
}

export function providerToLazy<T>(provider: Provider<T>): Lazy<T>;
export function providerToLazy<T>(provider?: Provider<T>): Lazy<T> | undefined;
export function providerToLazy<T>(provider?: Provider<T>): Lazy<T> | undefined {
  if (isUndefined(provider)) {
    return;
  }
  return new Lazy(() => provider.get());
}
