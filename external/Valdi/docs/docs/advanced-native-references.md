# Native References

## Introduction

Valdi allows you to pass callbacks and objects from platform (iOS/Android) to TypeScript and vice versa. The runtime maintains a mapping between those references, so that calling a TypeScript function that is backed by an Objective-C method ends up calling the underlying Objective-C method, and so that calling an Objective-C method or block that is backed by a TypeScript function ends up calling the underlying TypeScript function. The runtime automatically creates the references during the lifetime of your TypeScript component as it interacts with the platform bindings.

A C++, Objective-C or Kotlin object or function exposed to TypeScript is called a `Native Reference` within the runtime. A TypeScript function exposed to C++, Objective-C or Kotlin is called a `JS Value Reference`.

## Example

Here is a scenario where two kind of references would end up being created by the runtime:
```ts

// @ViewModel
// @ExportModel
interface ViewModel {
  fetchUserIds(cb: (userIds: string[]) => void): void;
}

// @ExportModel
export class MyComponent extends Component<ViewModel> {

  onCreate() {
    // By the time onCreate() is called, we have our view model provided
    // by platform. The ViewModel contains a callback called "fetchUserIds".
    // If the component was created from Kotlin, the "fetchUserIds" callback
    // inside the view model would be a reference to a Kotlin Function.
    const fetchUserIds = this.viewModel.fetchUserIds;

    // the "fetchUserIds" takes a callback as its parameter. If we call it from
    // TypeScript, we will pass it an arrow function. The runtime will create
    // a reference to the arrow function and put it inside a Kotlin Function.
    fetchUserIds((userIds) => {

    });
  }

}
```

The example above shows that when the component is created, it already has one `Native Reference` alive since the view model is natively generated and has a `fetchUserIds` function provided by platform. The `fetchUserIds` function takes a callback as its parameter, so when TypeScript calls it and provides a callback, the runtime creates a new `JS Value Reference` between TypeScript to platform to expose the TypeScript callback to platform.

## Lifecycle of references

The native and JS value references stay alive until either the receiving platform garbage collects the platform representation of the reference, or until the root component is destroyed.

For example, if `fetchUserIds` is implemented by Kotlin and that TypeScript calls that function and passes it a callback, on the Kotlin side the `fetchUserIds` will see the callback as a Kotlin Function object. Whenever the Kotlin Function object is garbage collected by the JVM, the `JS Value Reference` holding the TypeScript callback will be removed. If `fetchUserIds` is implemented by Objective-C, the callback for its parameter would be exposed as an Objective-C block, and the `JS Value Reference` for the callback would be removed as soon as Objective-C stops referencing the Objective-C block. The runtime tries to eliminate the emitted references as soon as it knows for sure that they are unused, to reduce memory usage during the lifetime of the component.

In order to reduce memory usage earlier whenever a root component is destroyed, the runtime also eagerly removes all the emitted references for that component. There could still be at that time some C++, Kotlin or Objective-C code that hold onto some TypeScript callbacks that were associated with the component. If C++, Kotlin or Objective-C calls into those disposed TypeScript callbacks, the runtime will emit a runtime error to notify about the issue.

## Debugging reference errors

As seen earlier in the `Lifecycle of references` section, references get eagerly disposed whenever the root component is destroyed, which could cause C++, Kotlin or Objective-C to hold into disposed `JS Value References`. When the runtime emits a reference, it also stores some debug information to help understanding what the reference is. The debug information gets shown in the runtime error message whenever something attempted to unwrap that disposed reference, which could happen if C++, Kotlin or Objective-C attempted to call a TypeScript function which was disposed. The debug information contains up to two string identifiers and up to 4 property chains. In the example from the `Example` section, the `fetchUserIds` callback would have a debug information like so:
```
context.fetchUserIds()
```
When TypeScript calls the `fetchUserIds` function and passes it a callback, the `JS Value Reference` emitted would have the following debug information:
```
context.fetchUserIds() -> <parameter 0>()
```
To make it human readable, we would read it from right to left. This could be read as `The reference is a callback which was the first parameter given to the context.fetchUserIds function`. If the `context.fetchUserIds` implementation was calling the TypeScript callback after the root component was destroyed, the error would look like:
```
Cannot unwrap JS value reference 'context.fetchUserIds() -> <parameter 0>()' as it was disposed
```
The fix to this issue could be to ensure that the `fetchUserIds` implementation does not call any given callback after the root component is destroyed. In Kotlin, this could be done by having the `fetchUserIds` implementation leverages some kind of disposable that would be disposed right before `rootView.destroy()` is called.

Here is an example of a more complex debug information error:
```
makeInterruptibleCallback -> <parameter 0>().getSamples() -> <parameter 1>()'
```
This could be read as `The reference is a function and the second parameter given to a call to getSamples(), which is a property of the first parameter given to makeInterruptibleCallback`

## Extending reference lifeycle

**If** your component needs to have some code that interacts with some bindings and absolutely needs to run even after the root component is is destroyed, you can temporarily extend the lifecycle of the references by calling `protectNativeRefs` from the `NativeReferences` module. The function returns a dispose callback which should be called once you no longer need the refs to be protected. Calling that function will prevent the references to be disposed until the returned value from  `protectNativeRefs` has been called. Here is an example of usage:
```ts
import { protectNativeRefs } from 'valdi_core/src/NativeReferences';

interface Context {
  userPreferences: UserPreferences;
  networkManager: NetworkManager;
}

export class MyComponent extends Component<{}, Context> {

  onDestroy() {
    this.doSave();
  }

  async doSave(): Promise<void> {
    // We need to be able to interact with our bindings for a bit longer after
    // we are destroyed, so we protect the native refs here
    const disposable = protectNativeRefs(this);
    try {
      const preferences = await this.context.userPreferences.save();
      await this.context.networkManager.saveUserPreferences(preferences);
    } finally {
      // We are done with our async calls, we can safely release the native refs
      disposable();
    }
  }
}
```

In this example, our component needs to interact asynchronously with some bindings in `onDestroy`. If we did not call `protectNativeRefs`, any callback/promise passed from TypeScript to the bindings would be not callable by the bindings after `onDestroy()` had finished executing. The first `await this.context.userPreferences.save()` would result in a `Cannot unwrap JS value reference` error as soon as the bindings implementation would call the callback. `protectNativeRefs` temporarily prevents the references to be eagerly destroyed while the `doSave()` asynchronous procedure is being done.

Please note that `protectNativeRefs` is inherently a bit of a dangerous API: you need to make sure that you call the returned disposable at some point otherwise the native references will leak, and you need to make sure the bindings will end up calling any callbacks your TypeScript code expects so that the disposable ends up being called. In the example above, if the promise returned by `this.context.userPreferences.save()` never got resolved by the `UserPreferences`, the `finally` block will never be reached so the disposable will never be called. As such, we recommend you only use this API if you absolutely need to perform a chain of asynchronous operations when your component is destroyed, similar to how an iOS developer might use Apple's `-[UIApplication beginBackgroundTaskWithExpirationHandler:]` API to perform a bit of work after an application is closed by the user.
