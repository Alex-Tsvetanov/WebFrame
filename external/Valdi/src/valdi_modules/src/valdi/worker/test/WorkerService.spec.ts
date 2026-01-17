import { toError } from 'valdi_core/src/utils/ErrorUtils';
import 'jasmine/src/jasmine';
import { IWorkerServiceClient } from 'worker/src/IWorkerService';
import {
  getWorkerServiceIdsForEntryPoint,
  startWorkerService,
  useOrStartWorkerService,
  useWorkerService,
} from 'worker/src/WorkerService';
import { CalculatorServiceEntryPoint, ICalculator } from './CalculatorService';

describe('WorkerService', () => {
  it('can be created and interacted with', async () => {
    const client: IWorkerServiceClient<ICalculator> = startWorkerService(CalculatorServiceEntryPoint, [0]);

    try {
      await client.api.add(2);
      await client.api.mul(3);
      await client.api.sub(1);
      await client.api.div(2);
      const result = await client.api.value();

      expect(result).toBe(2.5);
    } finally {
      client.dispose();
    }
  });

  it('can be provided values at init', async () => {
    const client: IWorkerServiceClient<ICalculator> = startWorkerService(CalculatorServiceEntryPoint, [42]);

    try {
      const result = await client.api.value();

      expect(result).toBe(42);
    } catch (err) {
      console.error(err);
      throw err;
    } finally {
      client.dispose();
    }
  });

  it('propagates error', async () => {
    const client: IWorkerServiceClient<ICalculator> = startWorkerService(CalculatorServiceEntryPoint, [0]);

    try {
      try {
        await client.api.div(0);
        // Should not reach this point
        expect(false).toBe(true);
      } catch (exc: unknown) {
        expect(toError(exc).message).toEqual('Division by zero is not allowed');
      }
    } finally {
      client.dispose();
    }
  });

  it('can be be retrieved by entry point', async () => {
    const client: IWorkerServiceClient<ICalculator> = startWorkerService(CalculatorServiceEntryPoint, [0]);

    try {
      const serviceIds = getWorkerServiceIdsForEntryPoint(CalculatorServiceEntryPoint);

      expect(serviceIds).toContain(client.serviceId);

      const newClient: IWorkerServiceClient<ICalculator> = useWorkerService(
        CalculatorServiceEntryPoint,
        client.serviceId,
      );

      expect(newClient).not.toBe(client);
      expect(newClient.api).toBe(client.api);

      await client.api.add(1000);

      // Value retrieved from the other client should match the changes
      // applied on the initial client
      const value = await newClient.api.value();
      expect(value).toEqual(1000);

      newClient.dispose();
    } finally {
      client.dispose();
    }
  });

  it('can can reuse worker service by matching arguments', async () => {
    const allClientsToDispose: IWorkerServiceClient<ICalculator>[] = [];
    try {
      const clientA: IWorkerServiceClient<ICalculator> = useOrStartWorkerService(CalculatorServiceEntryPoint, [0]);
      allClientsToDispose.push(clientA);

      const clientB: IWorkerServiceClient<ICalculator> = useOrStartWorkerService(CalculatorServiceEntryPoint, [1]);
      allClientsToDispose.push(clientB);

      // The clients should be different as the launch arguments were different
      expect(clientA.api).not.toBe(clientB.api);

      const clientABis: IWorkerServiceClient<ICalculator> = useOrStartWorkerService(CalculatorServiceEntryPoint, [0]);
      allClientsToDispose.push(clientABis);

      // Should match clientA but not clientB
      expect(clientABis.api).toBe(clientA.api);
      expect(clientABis.api).not.toBe(clientB.api);

      const clientBBis: IWorkerServiceClient<ICalculator> = useOrStartWorkerService(CalculatorServiceEntryPoint, [1]);
      allClientsToDispose.push(clientBBis);

      // Should match clientB but not clientA
      expect(clientBBis.api).toBe(clientB.api);
      expect(clientBBis.api).not.toBe(clientA.api);
    } finally {
      allClientsToDispose.forEach(client => client.dispose());
    }
  });

  it('disposes when last client is disposed', async () => {
    const client: IWorkerServiceClient<ICalculator> = startWorkerService(CalculatorServiceEntryPoint, [0]);

    try {
      const newClient: IWorkerServiceClient<ICalculator> = useWorkerService(
        CalculatorServiceEntryPoint,
        client.serviceId,
      );

      newClient.dispose();

      const serviceIds = getWorkerServiceIdsForEntryPoint(CalculatorServiceEntryPoint);

      expect(serviceIds).toContain(client.serviceId);
    } finally {
      client.dispose();
    }

    const serviceIds = getWorkerServiceIdsForEntryPoint(CalculatorServiceEntryPoint);

    expect(serviceIds).not.toContain(client.serviceId);
  });

  it('can cancel on-going calls', async () => {
    const client: IWorkerServiceClient<ICalculator> = startWorkerService(CalculatorServiceEntryPoint, [42]);

    try {
      // Test that an async call will be flushed by the time we complete another call
      let toStringResult: string | undefined;
      let error: unknown | undefined;
      client.api.valueToString().then(
        v => {
          toStringResult = v;
        },
        err => {
          error = err;
        },
      );

      await client.api.add(1);
      expect(error).toBeUndefined();
      expect(toStringResult).toBe('Value: 42');

      toStringResult = undefined;
      error = undefined;
      // Now call it but first cancel the promise
      const promise = client.api.valueToString();
      promise.then(
        v => {
          toStringResult = v;
        },
        err => {
          error = err;
        },
      );

      expect(promise.cancel).toBeDefined();
      promise.cancel!();

      const value = await client.api.value();

      expect(toStringResult).toBeUndefined();
      expect(error).toBeUndefined();
      expect(value).toBe(43);
    } finally {
      client.dispose();
    }
  });
});
