import { RequireFunc } from 'valdi_core/src/IModuleLoader';
import { IWorkerService } from './IWorkerService';
import { WorkerServiceEntryPointConstructor, IWorkerServiceEntryPoint } from './WorkerServiceEntryPoint';
import { unpromisifyObject } from './utils/WorkerServiceBridgeUtils';

export interface WorkerServiceExecutorTask {
  readonly serviceId: number;
  readonly file: string;
  readonly className: string;
  readonly args: readonly unknown[];
  readonly callback: (result: unknown, err: unknown) => void;
}

declare const require: RequireFunc;

function setupWorkerServiceServer(): void {
  onmessage = e => {
    processWorkerServiceExecutorTask(e.data as WorkerServiceExecutorTask);
  };
}

if (typeof postMessage !== 'undefined') {
  setupWorkerServiceServer();
}

export function processWorkerServiceExecutorTask(task: WorkerServiceExecutorTask): void {
  try {
    /* eslint-disable @typescript-eslint/no-unsafe-assignment */
    const module = require(task.file, true, true);
    /* eslint-disable @typescript-eslint/no-unsafe-member-access */
    const ctor = module[task.className] as WorkerServiceEntryPointConstructor<
      unknown,
      unknown[],
      IWorkerServiceEntryPoint<unknown, unknown[]>
    >;

    const entryPoint = new ctor();

    const result = entryPoint.start(...task.args);
    const unpromisifiedResult: IWorkerService<unknown> = {
      api: unpromisifyObject(result.api),
      dispose: result.dispose,
    };

    task.callback(unpromisifiedResult, undefined);
  } catch (exc) {
    task.callback(undefined, exc);
  }
}
