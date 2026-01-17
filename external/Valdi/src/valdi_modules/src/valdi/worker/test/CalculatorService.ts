import { CancelablePromise } from 'valdi_core/src/CancelablePromise';
import { IWorkerService } from 'worker/src/IWorkerService';
import { WorkerServiceEntryPoint, workerService } from 'worker/src/WorkerServiceEntryPoint';

export interface ICalculator {
  value(): Promise<number>;

  add(value: number): Promise<void>;
  sub(value: number): Promise<void>;
  mul(value: number): Promise<void>;
  div(value: number): Promise<void>;

  valueToString(): CancelablePromise<string>;
}

class CalculatorImpl implements ICalculator {
  private currentValue: number;

  constructor(initialValue: number) {
    this.currentValue = initialValue;
  }

  async add(value: number): Promise<void> {
    this.currentValue += value;
  }

  async sub(value: number): Promise<void> {
    this.currentValue -= value;
  }

  async mul(value: number): Promise<void> {
    this.currentValue *= value;
  }

  async div(value: number): Promise<void> {
    if (value === 0) {
      throw new Error('Division by zero is not allowed');
    }

    this.currentValue /= value;
  }

  async value(): Promise<number> {
    return this.currentValue;
  }

  async valueToString(): Promise<string> {
    return `Value: ${this.currentValue}`;
  }
}

@workerService('test_calculator', module)
export class CalculatorServiceEntryPoint extends WorkerServiceEntryPoint<ICalculator, [number]> {
  start(startingValue: number): IWorkerService<ICalculator> {
    return {
      api: new CalculatorImpl(startingValue),
      dispose() {},
    };
  }
}
