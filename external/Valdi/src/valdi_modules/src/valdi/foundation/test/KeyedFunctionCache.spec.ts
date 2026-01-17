import { KeyedFunctionCache } from 'foundation/src/KeyedFunctionCache';
import 'jasmine/src/jasmine';

describe('KeyedFunctionCache', () => {
  it('invokes with provided keys', () => {
    const fn = jasmine.createSpy();
    const cache = new KeyedFunctionCache<string, [number]>();
    const factory = cache.for(fn);

    factory.bindKey('foo')(5);
    factory.bindKey('bar')(6);

    expect(fn).toHaveBeenCalledWith('foo', 5);
    expect(fn).toHaveBeenCalledWith('bar', 6);
  });

  it('returns cached keyed functions', () => {
    const fn = jasmine.createSpy();
    const cache = new KeyedFunctionCache<string, [number]>();
    let factory = cache.for(fn);

    const fooCallback = factory.bindKey('foo');
    const barCallback = factory.bindKey('bar');

    factory = cache.for(fn);

    expect(factory.bindKey('foo')).toBe(fooCallback);
    expect(factory.bindKey('bar')).toBe(barCallback);
  });

  it('returns new keyed functions if provided function changes', () => {
    const fn = jasmine.createSpy();
    const cache = new KeyedFunctionCache<string, [number]>();
    let factory = cache.for(fn);

    const fooCallback = factory.bindKey('foo');
    const barCallback = factory.bindKey('bar');

    factory = cache.for(() => {});

    expect(factory.bindKey('foo')).not.toBe(fooCallback);
    expect(factory.bindKey('bar')).not.toBe(barCallback);
  });

  it('evicts unused keyed functions', () => {
    const fn = jasmine.createSpy();
    const cache = new KeyedFunctionCache<string, [number]>();
    let factory = cache.for(fn);

    const fooCallback = factory.bindKey('foo');
    const barCallback = factory.bindKey('bar');

    factory = cache.for(fn);

    expect(factory.bindKey('foo')).toBe(fooCallback);

    factory = cache.for(fn);

    expect(factory.bindKey('foo')).toBe(fooCallback);
    expect(factory.bindKey('bar')).not.toBe(fooCallback);
  });
});
