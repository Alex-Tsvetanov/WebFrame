import { BitmapAlphaType, BitmapColorType, BitmapInfo, ImageEncoding } from './IBitmap';
import { INativeBitmap } from './INativeBitmap';

export function createNativeBitmap(
  width: number,
  height: number,
  colorType: BitmapColorType,
  alphaType: BitmapAlphaType,
  rowBytes: number,
  buffer: ArrayBuffer | undefined,
): INativeBitmap;

export function decodeNativeBitmap(data: string | Uint8Array | ArrayBuffer): INativeBitmap;

export function disposeNativeBitmap(bitmap: INativeBitmap): void;

export function encodeNativeBitmap(bitmap: INativeBitmap, encoding: ImageEncoding, quality: number): ArrayBuffer;

export function lockNativeBitmap(bitmap: INativeBitmap): ArrayBuffer;
export function unlockNativeBitmap(bitmap: INativeBitmap): void;

export function getNativeBitmapInfo(bitmap: INativeBitmap): BitmapInfo;
