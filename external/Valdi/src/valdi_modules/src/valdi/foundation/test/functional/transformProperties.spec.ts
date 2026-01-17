import { transformProperties } from 'foundation/src/functional/transformProperties';
import 'jasmine/src/jasmine';

interface Foo {
  count: number;
  message: string;
}

describe('transformProperties', () => {
  it('applies transformation to specified properties', () => {
    const squareCount = transformProperties<Foo>({
      count: (x): number => x * x,
    });
    expect(squareCount({ count: 2, message: 'foo' })).toEqual({ count: 4, message: 'foo' });
    expect(squareCount({ count: 4, message: 'foo' })).toEqual({ count: 16, message: 'foo' });
  });
});
