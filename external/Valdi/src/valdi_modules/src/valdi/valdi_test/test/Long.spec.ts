import 'jasmine/src/jasmine';

describe('Long', () => {
  it('is available from global scope', () => {
    const long = Long.fromNumber(42);
    expect(long.toString()).toBe('42');
    expect(Long.isLong(long)).toBeTruthy();
  });

  it('can be converted into a primitive', () => {
    const long = Long.fromNumber(42);
    const value = (long as any) + 8;

    expect(value).toBe(50);
  });

  it('throws when converting large number into a primitive', () => {
    const smallNumber = Long.fromString('-999999999999999999999');
    const largeNumber = Long.fromString('999999999999999999999');
    expect(() => {
      return (smallNumber as any) + 8;
    }).toThrowError();

    expect(() => {
      return (largeNumber as any) + 8;
    }).toThrowError();
  });
});
