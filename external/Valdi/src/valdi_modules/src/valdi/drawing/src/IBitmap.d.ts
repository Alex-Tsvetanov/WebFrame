import { INativeBitmap } from './INativeBitmap';

export const enum BitmapColorType {
  UNKNOWN = 0,
  RGBA8888,
  BGRA8888,
  Alpha8,
  Gray8,
  RGBAF16,
  RGBAF32,
}

export const enum BitmapAlphaType {
  OPAQUE = 0,
  Premul,
  Unpremul,
}

export interface BitmapInfo {
  width: number;
  height: number;
  colorType: BitmapColorType;
  alphaType: BitmapAlphaType;
  rowBytes: number;
}

export const enum ImageEncoding {
  JPG = 0,
  PNG = 1,
  WEB = 2,
}

export interface IBitmap {
  native: INativeBitmap;

  dispose(): void;

  getInfo(): BitmapInfo;

  lockPixels(): ArrayBuffer;
  unlockPixels(): void;

  accessPixels<T>(cb: (view: DataView) => T): T;

  encode(encoding: ImageEncoding, quality: number): ArrayBuffer;
}
