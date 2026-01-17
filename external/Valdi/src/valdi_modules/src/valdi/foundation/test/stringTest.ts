import 'jasmine/src/jasmine';
import { slugifyUrlPath } from '../src/string';

describe('slugifyUrlPath', () => {
  it('returns a slugified URL path', () => {
    [
      ['search', 'search'],
      ['/search', 'search'],
      ['/some/long/path', 'somelongpath'],
    ].forEach(([input, expected]) => {
      expect(slugifyUrlPath(input)).toEqual(expected);
    });
  });
});
