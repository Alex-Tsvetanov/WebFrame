# Worker Service

### Introduction

Compiled TypeScript code runs in a JavaScript Virtual Machine using a single threaded event loop.

Expensive operations, like IO and parsing, are typically implemented by native code and run in a separate background thread, exposed to TypeScript as promise or callback based APIs. This allows for a fairly high level of concurrency even if the TypeScript code itself is single threaded.

Sometimes, that single thread might not be enough if there are some inherently expensive operations that are written in TypeScript which might compete with the Valdi components rendering the UI. To solve this problem, you can move parts or all of your business logic as a Worker Service, which will run in a background thread.

### Design

A Worker Service is a service that is started and registered from the TypeScript Main thread, but executes in a separate TypeScript Worker thread.

Throughout this page, we will refer to the code running in the TypeScript Main thread as the `host`, and the code running in the TypeScript Worker Thread as the `worker`.

The service exposes a promise based API, which can be used from the host. All the calls from host to the worker and vice versa are asynchronous. The service can be provided a set of dependencies. Each created service needs to be disposed at some point by calling the `dispose()` method from the host.

The service that executes in the worker uses an entirely separate memory heap than the host, passing objects from the host to the worker and vice versa will thus make the runtime to create a deep copy of those objects. There is therefore a cost to using a worker, and one must design their api carefully to limit "chattiness" between host and worker.

A Worker Service is declared with an entry point, which is a subclass of the `WorkerServiceEntryPoint` abstract class annotated with the `@workerService(module)` annotation. The entry point is responsible for creating the service and exposing its API by overriding the method `start()`, which will be called with the initialization arguments when the host requests to create the service. The returned object from the `start()` method will contain an `api` object, as well as a `dispose()` implementation which will be called when the service is requested to be torn down.

The host can start a worker service by using the `startWorkerService()` method, and pass as an argument the `WorkerServiceEntryPoint` subclass, as well as a tuple representing the initialization arguments, which will be forwarded to the `start()` method in the worker. The method returns a `IWorkerServiceClient` instance, containing the `api` that the worker service exposes so that the host can interact with the worker, a `serviceId` which uniquely identifies the service instance, and a `dispose()` method which should be called when the worker service is no longer needed. Failure to call `dispose()` will result in the service staying in memory forever.

### Usage

#### Declaration

To declare a worker service, one must first write a TypeScript interface representing its API. The API can contain any number of methods, but each method must return a promise, even if the result is void (in which case it'd return `Promise<void>`). Here is an example for a calculator:

```typescript
export interface ICalculator {
  value(): Promise<number>;

  add(value: number): Promise<void>;
  sub(value: number): Promise<void>;
  mul(value: number): Promise<void>;
  div(value: number): Promise<void>;
}
```

We then need an implementation for this interface, which will run in the worker. Here is an example:
```typescript
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
}
```

Lastly, we need to declare an entry point for this service by declaring a `WorkerServiceEntryPoint` subclass annotated with the `@workerService(module)` annotation:
```typescript
@workerService(module)
export class CalculatorServiceEntryPoint extends WorkerServiceEntryPoint<ICalculator, [number]> {
  start(startingValue: number): IWorkerService<ICalculator> {
    return {
      api: new CalculatorImpl(startingValue),
      dispose() {},
    };
  }
}
```

The first generic argument of `WorkerServiceEntryPoint` is the API type exposed, the second argument is a tuple representing the initialization arguments which will be passed to the `start()` method. An empty tuple can be used if starting the worker service requires no dependencies. In the example above, we pass a single dependency representing the `startingValue` of our calculator implementation.

#### Integration

To start using the worker service, we use the `startWorkerService()` to create it, which asynchronously creates the worker service in the worker, and synchromnously returns a client to that worker in the host. The host can immediately interact with the worker service through the `api` property. Once the host is done with the worker, the host should call `dispose()` on the client object.

Here is an example:
```typescript
// Start the worker service for our calculator, passing an initial value of 0
const client: IWorkerServiceClient<ICalculator> = startWorkerService(CalculatorServiceEntryPoint, [0]);

try {
  // Interact with the created worker service
  await client.api.add(2);
  await client.api.mul(3);
  await client.api.sub(1);
  await client.api.div(2);
  const result = await client.api.value();

  expect(result).toBe(2.5);
} finally {
  // Dispose the service when we're done
  client.dispose();
}
```

Note that in a real life situation, it is far more likely that a worker service would have a much longer lifecycle than the example shown above. A more typical integration might start the worker service from a root Valdi component and tear it down when the root component is destroyed.

### Limitations

As already mentioned, the worker uses a completely separate heap from the host, and objects are therefore copied.

When the host passes a function to the worker, that function is bridged, and a call to that function from the worker will be made asynchronously. As such, bridged TypeScript functions cannot return values synchronously. 

We recommend that you avoid passing TypeScript functions in general between host to worker and vice versa. You can however safely pass instances of `@ExportProxy` that are implemented by native Objective-C, Kotlin/Java or C++ code.

When passing an instance of a TypeScript class, only the own properties of the object will be copied, so methods implemented by the class won't be passed in. For example:
```typescript
class User {
  constructor(readonly firstName: string, readonly lastName: string) {}

  getName(): string {
      return `${this.firstName} ${this.lastName}`;
  }
}
```
If the host or worker passes an instance of `new User('John', 'Smith')`, the receiving side will get a plain object like:
```typescript
{
    firstName: 'John',
    lastName: 'Smith'
}
```
The `getName()` method won't be exported because it is not an own property of the `User` instance, and even if it were exported, calling it would not work as intended as the call would be made asynchronously, whereas the method expects to return a string synchronously.

Lastly, please note that by default, worker services run in a single shared thread. As such, code running in a worker service should avoid using the thread for too long and avoid processing unbounded tasks, as it will prevent other worker services to work properly.
