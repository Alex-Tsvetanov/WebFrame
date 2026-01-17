import { mapResult } from 'foundation/src/functional/mapResult';
import 'jasmine/src/jasmine';

describe('mapResult', () => {
  it('maps function return values', () => {
    const multiply = (x: number, y: number): number => x * y;
    const multiplyString = mapResult((x: number) => x.toString())(multiply);
    expect(multiplyString(4, 7)).toBe('28');
  });
});
