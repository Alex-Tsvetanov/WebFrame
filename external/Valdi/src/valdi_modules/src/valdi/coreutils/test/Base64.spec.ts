import { Base64 } from 'coreutils/src/Base64';
import 'jasmine/src/jasmine';

describe('coreutils > Base64', () => {
  // Byte array for '<<???>>'
  const byteArray = new Uint8Array([60, 60, 63, 63, 63, 62, 62]);
  it('should Base64 encode correctly', () => {
    expect(Base64.fromByteArray(byteArray)).toEqual('PDw/Pz8+Pg==');
  });
  it('should url-safe Base64 encode correctly', () => {
    expect(Base64.fromByteArray(byteArray, { urlSafe: true })).toEqual('PDw_Pz8-Pg');
  });
  it('should Base64 decode correctly with padding', () => {
    expect(Base64.toByteArray('PDw/Pz8+Pg==')).toEqual(byteArray);
  });
  it('should Base64 decode correctly without padding', () => {
    expect(Base64.toByteArray('PDw/Pz8+Pg')).toEqual(byteArray);
  });
  it('should url-safe Base64 decode correctly without padding', () => {
    expect(Base64.toByteArray('PDw_Pz8-Pg')).toEqual(byteArray);
  });
  it('should url-safe Base64 decode correctly with padding', () => {
    expect(Base64.toByteArray('PDw_Pz8-Pg=')).toEqual(byteArray);
  });
});
