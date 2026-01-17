export interface KeyedFunctionFactoryProvider<Key, Parameters extends any[] = [], ReturnValue = void> {
  /**
   * Creates an instance of {@link KeyedFunctionFactory} that uses the provided function to make keyed versions
   * @param fn function that accepts a key + parameters, used to generated keyed functions
   */
  for(fn: (key: Key, ...params: Parameters) => ReturnValue): KeyedFunctionFactory<Key, Parameters, ReturnValue>;
}

export interface KeyedFunctionFactory<Key, Parameters extends any[], ReturnValue> {
  bindKey(key: Key): (...params: Parameters) => ReturnValue;
}

/**
 * Provides callback functions for lists of Components such that the callback functions for each Component
 * in the list are reused if the Component's key remains the same. This helps avoid unnecessary re-renders
 * due to view model changes caused by new function instances.
 * @example
 * interface ItemsViewModel {
 *   items: string[];
 *   onItemTap(i: number): void;
 * }
 * class ItemsView extends Component<ItemsViewModel> {
 *   private readonly onItemTapCache = new KeyedFunctionCache<number>();
 *   onRender(): void {
 *     const onItemTapFactory = onItemTapCache.for(this.viewModel.onItemTap);
 *     this.viewModel.items.forEach((item, i) => {
 *       <label value={item} onTap={onItemTapFactory.bindKey(i)} />;
 *     });
 *   }
 * }
 */
export class KeyedFunctionCache<Key, Parameters extends any[] = [], ReturnValue = void>
  implements KeyedFunctionFactoryProvider<Key, Parameters, ReturnValue>
{
  private cache = new Map<Key, (...params: Parameters) => ReturnValue>();
  private lastUsedFn?: (key: Key, ...params: Parameters) => ReturnValue;

  for(fn: (key: Key, ...params: Parameters) => ReturnValue): KeyedFunctionFactory<Key, Parameters, ReturnValue> {
    let prevCache: ReadonlyMap<Key, (...params: Parameters) => ReturnValue>;
    if (this.lastUsedFn === fn) {
      prevCache = this.cache;
    } else {
      // The function itself updated so the cached keyed functions are no longer valid
      prevCache = new Map();
    }
    // Clear the current cache to avoid holding stale references
    this.cache = new Map();
    this.lastUsedFn = fn;
    return new CachingKeyedFunctionFactory(prevCache, this.cache, fn);
  }
}

class CachingKeyedFunctionFactory<Key, Parameters extends any[], ReturnValue = void> {
  constructor(
    private readonly prevCache: ReadonlyMap<Key, (...params: Parameters) => ReturnValue>,
    private readonly newCache: Map<Key, (...params: Parameters) => ReturnValue>,
    private readonly fn: (key: Key, ...params: Parameters) => ReturnValue,
  ) {}

  /**
   * Either returns a previously generated keyed function or creates a new one if none exists
   * for the provided key. Caches any requested keyed functions in the {@link newCache}
   * @param key the unique key that identifies a Component within a list
   * @returns a callback function with the key parameter removed
   */
  bindKey(key: Key): (...params: Parameters) => ReturnValue {
    const keyedFn = this.prevCache.get(key) ?? this.fn.bind(null, key);
    this.newCache.set(key, keyedFn);
    return keyedFn;
  }
}
