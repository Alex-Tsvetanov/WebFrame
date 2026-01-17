# Setup for development
In the last section, we generated a new valdimodule. Now, let's hook it all up to iOS.
We will create a new option in the Settings menu from scratch and will load a new view when it is clicked.

If you're continuing directly from the last section. You'll need to reset the Playground.tsx file back to the original to prevent compilation errors. We won't be using the Playground module in this part.

## Building your component for Swift
In the module.yaml of the module, add `language: swift` to the `ios` section. The module.yaml should contain the following:

```bazel
ios:
  class_prefix: SCCHelloWorld
  module_name: SCCHelloWorld
  language: swift

```

**TODO**: Update this for open source
