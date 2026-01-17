import { runWith } from 'foundation/src/runWith';
import 'jasmine/src/jasmine';

describe('runWith', () => {
  it('runs with input', () => {
    const fn = jasmine.createSpy();
    runWith(123, fn);
    expect(fn).toHaveBeenCalledWith(123);
  });
});
