/**
 * Protocol:
 * Every entry is a 32 bits integer.
 * [type][nodeId][data...]
 *
 * Create element: [1][nodeId][viewClass]
 * Destroy element: [2][nodeId]
 * Set root element: [3][nodeId]
 * Move element to parent: [4][nodeId][parentId][index]
 * Set element attribute undefined: [5][nodeId][name]
 * Set element attribute null: [6][nodeId][name]
 * Set element attribute boolean: [7][nodeId][name][value]
 * Set element attribute number: [8][nodeId][name][value 64bits]
 * Set element attribute array: [9][nodeId][name][size][valueIndexes...]
 * Set element attribute unknown: [10][nodeId][name][valueIndex]
 * Begin animation: [11][duration 64bits][curve][beginFromCurrentState][controlPointsValueIndex]
 * End animation: [12]
 */

export interface RenderRequest {
  treeId: string;
  descriptor: ArrayBuffer;
  descriptorSize: number;
  values: any[];
  visibilityObserver?: NativeVisibilityObserver;
  frameObserver?: NativeFrameObserver;
}

export type NativeVisibilityObserver = (
  disappearingElements: number[],
  appearingElements: number[],
  viewportUpdates: number[],
  acknowledger: () => void,
  eventTime: number,
) => void;
export type NativeFrameObserver = (updates: Float64Array) => void;
