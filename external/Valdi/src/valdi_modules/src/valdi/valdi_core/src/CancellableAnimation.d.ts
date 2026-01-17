export type CancelToken = number;

export const enum CancellableAnimationState {
  Running,
  Cancelled,
  Finished,
}

export interface CancellableAnimation {
  cancel(): void;
  getState(): CancellableAnimationState;
  start(): Promise<boolean>;
}
