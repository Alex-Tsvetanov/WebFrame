import { uuidStringToBytes, uuidBytesToString } from '../src/uuidUtils';
import 'jasmine/src/jasmine';

const UUIDS: { uuidString: string; uuidBytes: Uint8Array }[] = [
  {
    uuidString: 'f64ea4e0-bc3b-42c2-ad34-29698bafef09',
    uuidBytes: Uint8Array.from([246, 78, 164, 224, 188, 59, 66, 194, 173, 52, 41, 105, 139, 175, 239, 9]),
  },
  {
    uuidString: 'f47ac10b-58cc-0372-8567-0e02b2c3d479',
    uuidBytes: Uint8Array.from([244, 122, 193, 11, 88, 204, 3, 114, 133, 103, 14, 2, 178, 195, 212, 121]),
  },
];

describe('uuidStringToBytes', () => {
  it('should convert a uuid string to correct bytes', () => {
    for (const uuid of UUIDS) {
      const { uuidString, uuidBytes } = uuid;
      expect(uuidStringToBytes(uuidString)).toEqual(uuidBytes);
    }
  });

  it('should convert to and from a string', () => {
    for (const uuid of UUIDS) {
      const { uuidString } = uuid;
      expect(uuidStringToBytes(uuidString)).toEqual(
        uuidStringToBytes(uuidBytesToString(uuidStringToBytes(uuidString))),
      );
    }
  });
});

describe('uuidBytesToString', () => {
  it('should convert uuid bytes to string', () => {
    for (const uuid of UUIDS) {
      const { uuidString, uuidBytes } = uuid;
      expect(uuidBytesToString(uuidBytes)).toEqual(uuidString);
    }
  });

  it('should convert to and from bytes', () => {
    for (const uuid of UUIDS) {
      const { uuidBytes } = uuid;
      expect(uuidBytesToString(uuidBytes)).toEqual(uuidBytesToString(uuidStringToBytes(uuidBytesToString(uuidBytes))));
    }
  });
});
