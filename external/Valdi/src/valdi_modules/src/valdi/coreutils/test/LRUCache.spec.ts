import { LRUCache } from 'coreutils/src/LRUCache';
import 'jasmine/src/jasmine';

describe('LRUCache', () => {
  it('should update items order on get', () => {
    const cache = new LRUCache<string>(8);
    cache.insert('1', 'Hello');
    cache.insert('2', 'Bonjour');
    cache.insert('3', 'Dzien dobry');

    expect(cache.all()).toEqual(['Dzien dobry', 'Bonjour', 'Hello']);

    let fetchedItem = cache.get('2');
    expect(fetchedItem).toBe('Bonjour');

    expect(cache.all()).toEqual(['Bonjour', 'Dzien dobry', 'Hello']);

    // No change expected if we fetch the item at the front
    fetchedItem = cache.get('2');
    expect(fetchedItem).toBe('Bonjour');

    expect(cache.all()).toEqual(['Bonjour', 'Dzien dobry', 'Hello']);

    // Fetching last item should re-order

    fetchedItem = cache.get('1');
    expect(fetchedItem).toBe('Hello');

    expect(cache.all()).toEqual(['Hello', 'Bonjour', 'Dzien dobry']);
  });

  it('should trim items on out of bounds', () => {
    const cache = new LRUCache<string>(2);
    cache.insert('1', 'Hello');
    cache.insert('2', 'Bonjour');
    cache.insert('3', 'Dzien dobry');

    expect(cache.all()).toEqual(['Dzien dobry', 'Bonjour']);

    cache.insert('4', 'Hola');

    expect(cache.all()).toEqual(['Hola', 'Dzien dobry']);
  });

  it('should replace item on re-insert', () => {
    const cache = new LRUCache<string>(8);
    cache.insert('1', 'Hello');
    cache.insert('2', 'Bonjour');
    cache.insert('3', 'Dzien dobry');

    expect(cache.all()).toEqual(['Dzien dobry', 'Bonjour', 'Hello']);

    // Replace Hello by Hola
    cache.insert('1', 'Hola');

    expect(cache.all()).toEqual(['Hola', 'Dzien dobry', 'Bonjour']);
  });

  it('can erase item', () => {
    const cache = new LRUCache<string>(8);
    cache.insert('1', 'Hello');
    cache.insert('2', 'Bonjour');
    cache.insert('3', 'Dzien dobry');

    expect(cache.all()).toEqual(['Dzien dobry', 'Bonjour', 'Hello']);

    cache.remove('2');

    expect(cache.all()).toEqual(['Dzien dobry', 'Hello']);

    cache.remove('1');

    expect(cache.all()).toEqual(['Dzien dobry']);

    cache.remove('3');

    expect(cache.all()).toEqual([]);
  });

  it('can be cleared', () => {
    const cache = new LRUCache<string>(8);
    cache.insert('1', 'Hello');
    cache.insert('2', 'Bonjour');
    cache.insert('3', 'Dzien dobry');

    expect(cache.all()).toEqual(['Dzien dobry', 'Bonjour', 'Hello']);

    cache.clear();

    expect(cache.all()).toEqual([]);
  });
});
