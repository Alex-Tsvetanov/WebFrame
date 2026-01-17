import { PromiseOnCancelFn } from 'valdi_core/src/CancelablePromise';

export interface ServiceError {
  message: string;
  stack?: string;
}

export const enum ServiceCallbackType {
  Success = 1,
  Error = 2,
  ForwardCanceler = 3,
}

export type ServiceCallback<T> = (
  data: T | PromiseOnCancelFn | undefined,
  error: ServiceError | undefined,
  type: ServiceCallbackType,
) => void;
