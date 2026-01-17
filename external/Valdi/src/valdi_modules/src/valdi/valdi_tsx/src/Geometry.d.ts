/**
 * @ExportModel({
 *   ios: 'SCPoint',
 *   android: 'com.snap.client.valdi.Point'
 * })
 */
export interface Point {
  x: number;
  y: number;
}

/**
 * @ExportModel({
 *   ios: 'SCSize',
 *   android: 'com.snap.client.valdi.Size'
 * })
 */
export interface Size {
  width: number;
  height: number;
}

export interface Vector {
  dx: number;
  dy: number;
}

/**
 * @ExportModel({
 *   ios: 'SCElementFrame',
 *   android: 'com.snap.client.valdi.ElementFrame'
 * })
 */
export interface ElementFrame {
  x: number;
  y: number;
  width: number;
  height: number;
}
