export interface ICompilationCache {
  close(): void;

  evictEntriesBeforeTime(lastUsedTimestamp: number): void;

  getCachedEntry(path: string, container: string, hash: string): Promise<string | undefined>;
  setCachedEntry(path: string, container: string, hash: string, data: string): Promise<void>;
}
