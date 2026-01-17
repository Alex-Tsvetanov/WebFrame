interface LRUCacheEntry<T> {
  key: string;
  item: T;
  previous: LRUCacheEntry<T> | undefined;
  next: LRUCacheEntry<T> | undefined;
}

function checkMaxSize(maxSize: number) {
  if (maxSize < 0) {
    throw new Error(`Invalid maxSize ${maxSize}`);
  }
}

export interface LRUCacheListener<T> {
  onItemEvicted(key: string, item: T): void;
}

/**
 * A simple LRU cache that holds up to N items and will
 * evict the least recently used ones.
 */
export class LRUCache<T> {
  listener?: LRUCacheListener<T>;

  get size(): number {
    return this.entryById.size;
  }

  private entryById = new Map<string, LRUCacheEntry<T>>();
  private maxSize: number;
  private head?: LRUCacheEntry<T>;
  private tail?: LRUCacheEntry<T>;

  constructor(maxSize: number) {
    checkMaxSize(maxSize);
    this.maxSize = maxSize;
  }

  /**
   * Set the maximum number of items that the LRU cache can hold.
   */
  setMaxSize(maxSize: number) {
    checkMaxSize(maxSize);

    if (this.maxSize !== maxSize) {
      this.maxSize = maxSize;
      this.trimToMaxSize(maxSize);
    }
  }

  /**
   * Return the item for the given key.
   */
  get(key: string): T | undefined {
    const entry = this.entryById.get(key);
    if (!entry) {
      return undefined;
    }

    this.removeEntryFromList(entry);
    this.pushEntryInList(entry);

    return entry.item;
  }

  /**
   * Insert an item with the given key.
   */
  insert(key: string, item: T): void {
    let shouldTrim = false;
    let entry = this.entryById.get(key);
    if (!entry) {
      entry = {
        key,
        item,
        previous: undefined,
        next: undefined,
      };
      this.entryById.set(key, entry);
      shouldTrim = this.entryById.size > this.maxSize;

      this.pushEntryInList(entry);
    } else {
      entry.item = item;

      this.removeEntryFromList(entry);
      this.pushEntryInList(entry);
    }

    if (shouldTrim) {
      this.trimToMaxSize(this.maxSize);
    }
  }

  /**
   * Returns whether the cache holds an item for the given key
   * @param key
   * @returns
   */
  has(key: string): boolean {
    return this.entryById.has(key);
  }

  /**
   * Remove the item with the given key. Return true if the item was found
   * and removed, or false otherwise.
   */
  remove(key: string): boolean {
    const entry = this.entryById.get(key);
    if (!entry) {
      return false;
    }

    this.removeEntry(entry);
    return true;
  }

  /**
   * Remove all of the items within the cache.
   */
  clear(): void {
    this.trimToMaxSize(0);
  }

  /**
   * Returns all the items that the LRUCache holds.
   */
  all(): T[] {
    const output: T[] = [];
    let current = this.head;

    while (current) {
      output.push(current.item);
      current = current.next;
    }

    return output;
  }

  private trimToMaxSize(maxSize: number) {
    while (this.entryById.size > maxSize) {
      const tail = this.tail!;
      this.removeEntry(tail);
    }
  }

  private removeEntry(entry: LRUCacheEntry<T>) {
    this.entryById.delete(entry.key);
    this.removeEntryFromList(entry);

    if (this.listener) {
      this.listener.onItemEvicted(entry.key, entry.item);
    }
  }

  private removeEntryFromList(entry: LRUCacheEntry<T>) {
    const previous = entry.previous;
    const next = entry.next;

    if (previous) {
      previous.next = next;
    }
    if (next) {
      next.previous = previous;
    }

    if (this.head === entry) {
      this.head = next;
    }

    if (this.tail === entry) {
      this.tail = previous;
    }

    entry.previous = undefined;
    entry.next = undefined;
  }

  private pushEntryInList(entry: LRUCacheEntry<T>) {
    const next = this.head;

    if (next) {
      next.previous = entry;
    }

    entry.next = next;

    this.head = entry;

    if (!this.tail) {
      this.tail = entry;
    }
  }
}
