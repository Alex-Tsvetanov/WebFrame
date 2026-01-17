import 'ts-jest';
import {
  COMPRESSION_BYTES_THRESHOLD,
  SQLiteCompilationCache,
  SQLiteCompilationCacheTimestampProvider,
} from './SQLiteCompilationCache';
class TestTimestampProvider implements SQLiteCompilationCacheTimestampProvider {
  currentTimestamp = 0;

  getCurrentTimestamp(): number {
    return this.currentTimestamp;
  }
}

describe('SQLiteCompilationCache', () => {
  function cacheIt(
    name: string,
    cb: (cache: SQLiteCompilationCache, timestampProvider: TestTimestampProvider) => Promise<void>,
  ): void {
    it(name, async () => {
      const timestampProvider = new TestTimestampProvider();
      const cache = SQLiteCompilationCache.newInMemoryInstance(timestampProvider);

      try {
        await cb(cache, timestampProvider);
      } finally {
        cache.close();
      }
    });
  }

  cacheIt('can insert and get values', async (cache) => {
    let result = await cache.getCachedEntry('files', 'file.ts', 'xxx');
    expect(result).toBeUndefined();

    await cache.setCachedEntry('files', 'file.ts', 'xxx', 'Hello World');
    await cache.setCachedEntry('files', 'file.ts', 'yyy', 'Goodbye World');

    result = await cache.getCachedEntry('files', 'file.ts', 'xxx');
    expect(result).toEqual('Hello World');
    result = await cache.getCachedEntry('files', 'file.ts', 'yyy');
    expect(result).toEqual('Goodbye World');
  });

  cacheIt('updates last access timestamp on get', async (cache, timestampProvider) => {
    timestampProvider.currentTimestamp = 1;

    await cache.setCachedEntry('files', 'file.ts', 'xxx', 'Hello World');

    let entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(1);

    expect(entries[0].last_access_date).toBe(1);

    timestampProvider.currentTimestamp = 2;

    const entry = await cache.getCachedEntry('files', 'file.ts', 'xxx');
    expect(entry).toEqual('Hello World');

    cache.flushDeferredStatements();

    entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(1);

    expect(entries[0].last_access_date).toBe(2);
  });

  cacheIt('can evict old entries', async (cache, timestampProvider) => {
    timestampProvider.currentTimestamp = 1;
    await cache.setCachedEntry('files', 'file.ts', 'xxx', 'Hello World');
    timestampProvider.currentTimestamp = 2;
    await cache.setCachedEntry('files', 'file.ts', 'yyy', 'Goodbye World');
    timestampProvider.currentTimestamp = 3;
    await cache.setCachedEntry('files', 'file.ts', 'zzz', 'Yepyep');

    let entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(3);

    cache.evictEntriesBeforeTime(2);

    entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(2);

    cache.evictEntriesBeforeTime(3);

    entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(1);
    expect(entries[0].data.toString('utf-8')).toEqual('Yepyep');

    cache.evictEntriesBeforeTime(4);

    entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(0);
  });

  cacheIt('compresses when data goes beyond the compression threshold', async (cache) => {
    let data = '';
    for (let i = 0; i < COMPRESSION_BYTES_THRESHOLD; i++) {
      data += 'a';
    }
    await cache.setCachedEntry('files', 'file.ts', 'xxx', data);

    let entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(1);

    expect(entries[0].data.toString('utf-8')).toEqual(data);

    // Go beyond the threshold where we start compressing
    data += 'z';
    await cache.setCachedEntry('files', 'file.ts', 'xxx', data);

    entries = cache.getAllCachedEntries();
    expect(entries.length).toBe(1);

    expect(entries[0].data.toString('utf-8')).not.toEqual(data);

    const fetched = await cache.getCachedEntry('files', 'file.ts', 'xxx');
    expect(fetched).toBeDefined();
    expect(fetched).toEqual(data);
  });
});
