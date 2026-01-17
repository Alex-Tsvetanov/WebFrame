/**
 * @ExportModel({ios: 'SCValdiFoundationLong', android: 'com.snap.valdi.foundation.Long'})
 */
export interface NativeLong {
  /** Unsigned int32 */
  readonly lowBits: number;
  /** Unsigned int32 */
  readonly highBits: number;
}

/** Converts a native long into a js long */
export function fromNativeLong(long: NativeLong, unsigned: boolean): Long {
  // Need to convert the unsigned bits into signed bits for the js long class
  return Long.fromBits(long.lowBits >> 0, long.highBits >> 0, unsigned);
}

/** Converts a js long into a native long */
export function toNativeLong(long: Long): NativeLong {
  // Need to convert the signed bits from js long into unsigned bits for native long
  return { lowBits: long.low >>> 0, highBits: long.high >>> 0 };
}
