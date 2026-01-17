import { IWorkerService } from './IWorkerService';

/* eslint-disable @typescript-eslint/no-explicit-any */
type AnyConstructor<A = object> = new (...input: any[]) => A;

export interface IWorkerServiceEntryPoint<T, A extends unknown[]> {
  start(...args: A): IWorkerService<T>;
}

export abstract class WorkerServiceEntryPoint<T, A extends unknown[]> implements IWorkerServiceEntryPoint<T, A> {
  static file: string | undefined;
  static executorIdentifier: string | undefined;

  abstract start(...args: A): IWorkerService<T>;
}

export declare type WorkerServiceEntryPointConstructor<
  T,
  A extends unknown[],
  EntryPoint extends WorkerServiceEntryPoint<T, A>,
> = {
  file: string | undefined;
  executorIdentifier: string | undefined;

  new (): EntryPoint;
};

export const workerService =
  (executorIdentifier: string, module: { path: string }) =>
  <T extends AnyConstructor<IWorkerServiceEntryPoint<any, any>>>(base: T) => {
    const newClass = class extends base {
      static file: string;
      static executorIdentifier: string;
    };
    newClass.file = module.path;
    newClass.executorIdentifier = executorIdentifier;
    Object.defineProperty(newClass, 'name', { value: base.name });

    return newClass;
  };
