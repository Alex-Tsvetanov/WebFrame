import { md5 } from 'coreutils/src/md5';
import 'jasmine/src/jasmine';

describe('coreutils > md5', () => {
  it('should md5 hash correctly', () => {
    expect(md5.hash('hello')).toEqual('5d41402abc4b2a76b9719d911017c592');
    expect(md5.hash('W7_EDlXWTBiXAEEniNoMPwAAYZXVhZ2NqcGt6AYFPU_XMAYFPU_WKAAAAAA')).toEqual(
      '15b244225a3207159403fdc623aa4991',
    );
  });
});
