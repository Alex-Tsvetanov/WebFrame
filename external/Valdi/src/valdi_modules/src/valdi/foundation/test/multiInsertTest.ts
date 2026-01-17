import { multiInsert } from 'foundation/src/array';
import 'jasmine/src/jasmine';

describe('multiInsert', () => {
  const arr = ['a', 'b', 'c', 'd'];
  const values = { 0: '1', 1: '2', 2: '3' };
  it('inserts multiple values', () => {
    expect(multiInsert(arr, values)).toEqual(['1', 'a', '2', 'b', '3', 'c', 'd']);
  });

  it('empty values object', () => {
    expect(multiInsert(arr, {})).toEqual(arr);
  });
});
