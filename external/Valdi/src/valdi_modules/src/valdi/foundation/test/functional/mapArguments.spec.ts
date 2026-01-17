import { mapArguments } from 'foundation/src/functional/mapArguments';
import 'jasmine/src/jasmine';

describe('mapArguments', () => {
  it('maps function arguments', () => {
    const concat = (x: string, y: string): string => x + y;
    const concatNumbers = mapArguments((i: number, j: number): [string, string] => [i.toString(), j.toString()])(
      concat,
    );
    expect(concatNumbers(123, 456)).toEqual('123456');
  });
});
