# Setup for development
In the last section, we generated a new Valdi module. Now, let's hook it all up to Android.

## Dependencies

**TODO**: is this relevant to `open_source`?

In order to use the TypeScript business logic, we need to build a reference to the functions we just generated. For Valdi with no UI, we need a reference to the `ValdiJSRuntime`. We can get the `ValdiJSRuntime` through the `IValdiRuntime`. **TODO**: Wo we actually have `IValdiRuntime` now?

In Android, the `IValdiRuntime` is injected by Dagger. We're building this example in part of the code base where the dependencies have already been setup. When you're building your own code, you'll need to set up Dagger.


**TODO**: Example of providing "headless" `valdimodule` APIs to other parts of Android.

### [Next >](./2-android_hook_up_module.md)
