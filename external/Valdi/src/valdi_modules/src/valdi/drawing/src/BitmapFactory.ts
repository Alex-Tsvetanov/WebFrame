import * as BitmapNative from './BitmapNative';
import { BitmapInfo, IBitmap, ImageEncoding } from './IBitmap';
import { INativeBitmap } from './INativeBitmap';

class BitmapImpl implements IBitmap {
  constructor(readonly native: INativeBitmap) {}

  getInfo(): BitmapInfo {
    return BitmapNative.getNativeBitmapInfo(this.native);
  }

  dispose(): void {
    BitmapNative.disposeNativeBitmap(this.native);
  }

  lockPixels(): ArrayBuffer {
    return BitmapNative.lockNativeBitmap(this.native);
  }

  unlockPixels(): void {
    BitmapNative.unlockNativeBitmap(this.native);
  }

  accessPixels<T>(cb: (view: DataView) => T): T {
    const pixels = this.lockPixels();
    try {
      return cb(new DataView(pixels));
    } finally {
      this.unlockPixels();
    }
  }

  encode(encoding: ImageEncoding, quality: number): ArrayBuffer {
    return BitmapNative.encodeNativeBitmap(this.native, encoding, quality);
  }
}

/**
 * Create a new blank writable bitmap with the given bitmap info.
 * This will allocate the underlying bitmap buffer.
 */
export function createBitmap(info: BitmapInfo): IBitmap {
  return new BitmapImpl(
    BitmapNative.createNativeBitmap(info.width, info.height, info.colorType, info.alphaType, info.rowBytes, undefined),
  );
}

/**
 * Create a new bitmap which will write into the given buffer.
 */
export function createBitmapWithBuffer(info: BitmapInfo, buffer: ArrayBuffer): IBitmap {
  return new BitmapImpl(
    BitmapNative.createNativeBitmap(info.width, info.height, info.colorType, info.alphaType, info.rowBytes, buffer),
  );
}

export function wrapBitmap(native: INativeBitmap): IBitmap {
  return new BitmapImpl(native);
}

/**
 * Returns a new bitmap by decoding the given image data provided as a base64 string
 * or as a buffer.
 */
export function decodeBitmap(data: string | Uint8Array | ArrayBuffer): IBitmap {
  return new BitmapImpl(BitmapNative.decodeNativeBitmap(data));
}
