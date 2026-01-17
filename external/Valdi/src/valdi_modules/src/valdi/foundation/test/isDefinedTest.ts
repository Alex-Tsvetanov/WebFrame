import 'jasmine/src/jasmine';
import { isDefined } from 'foundation/src/isDefined';

describe('optional', () => {
  it('empty val', () => {
    let val: boolean | undefined;
    expect(isDefined(val)).toBe(false);
  });

  it('true val', () => {
    const val: boolean | undefined = true;
    expect(isDefined(val)).toBe(true);
  });

  it('false val', () => {
    const val: boolean | undefined = false;
    expect(isDefined(val)).toBe(true);
  });

  it('filter', () => {
    const vals = ['abc', undefined, 1, 2, null, 3];
    expect(vals.filter(isDefined)).toEqual(['abc', 1, 2, 3]);
  });
});
