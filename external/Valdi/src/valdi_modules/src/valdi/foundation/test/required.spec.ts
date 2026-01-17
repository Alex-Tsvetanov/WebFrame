import 'jasmine/src/jasmine';
import { required } from 'foundation/src/required';

describe('required', () => {
  it('returns defined values', () => {
    expect(required({})).toEqual({});
    expect(required({ foo: 'bar' })).toEqual({ foo: 'bar' });
    expect(required(500)).toBe(500);
    expect(required(true)).toBe(true);
    expect(required('blah')).toBe('blah');
  });

  it('returns falsy defined values', () => {
    expect(required(0)).toBe(0);
    expect(required('')).toBe('');
    expect(required(null)).toBe(null);
    expect(required(false)).toBe(false);
  });

  it('throws for undefined', () => {
    try {
      expect(required(undefined));
      fail('unexpected success');
    } catch (e) {
      expect(e).toEqual(new TypeError());
    }
  });

  it('throws with message', () => {
    try {
      expect(required(undefined, 'uh oh!'));
      fail('unexpected success');
    } catch (e) {
      expect(e).toEqual(new TypeError('uh oh!'));
    }
  });
});
