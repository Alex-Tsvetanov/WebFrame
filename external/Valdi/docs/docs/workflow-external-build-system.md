# CocoaPods, SwiftPM, Xcode and Gradle integration

## Introduction

Valdi can be integrated inside a separate build system. This document covers how to integrate a set of Valdi modules into CocoaPods, SwiftPM, Xcode and Gradle. The main trick behind these integrations is that you can build Valdi modules into a shared iOS and Android framework, that can be added into an iOS and Android project. The built framework is self contained: it has the compiled Valdi modules including compiled TS sources, as well as the Valdi runtime itself.

## Prequisites

To export a set of Valdi modules, you need to define a `valdi_exported_library()` target that depends on all of the Valdi modules you wish to include in this library.

```python
load("@valdi//bzl/valdi:valdi_exported_library.bzl", "valdi_exported_library")

valdi_exported_library(
    name = "hello_world_export",
    ios_bundle_id = "com.snap.hello_world.lib",
    ios_bundle_name = "HelloWorld",
    deps = ["//apps/helloworld/src/valdi/hello_world"],
)

```

`deps` can contain a single Valdi module, or multiple.

## iOS

You can export your project as a xcframework for iOS using the `valdi export ios` command:

```sh
valdi export ios --output_path my_framework.xcframework
```

This will build your Valdi modules defined from the `valdi_exported_library()` target, as well as the Valdi runtime, and package it into a universal framework that works for iOS simulator and real devices.
Note that by default, the library is compiled in debug module. You use the `--build_config release` to build in release mode:

```sh
valdi export ios --build_config release --output_path my_framework.xcframework
```

### Xcode

To use your `.xcframework` in your Xcode project, just drag and drop the built `.xcframework` file into Xcode. You should be able to import it and use the generated classes from it.

```swift
import UIKit
import HelloWorld

class ViewController: UIViewController {

    private var runtimeManager: SCValdiRuntimeManager?

    override func loadView() {
        // For best performance, your app should maintain a singleton
        // of the Valdi runtime manager.
        let runtimeManager = SCValdiRuntimeManager()
        self.runtimeManager = runtimeManager

        self.view = SCCHelloWorldApp(viewModel: nil, componentContext: nil, runtime: runtimeManager.mainRuntime)
    }
}
```

### SwiftPM

To use your `.xcframework` in SwiftPM, you can define a `Package.swift` that exposes your `.xcframework`. Here is an example of such a file:

```swift
// swift-tools-version:5.6

import PackageDescription

let package = Package(
    name: "HelloWorld",
    platforms: [
        .iOS(.v12)
    ],
    products: [
        .library(name: "HelloWorld", targets: ["HelloWorldFramework"])
    ],
    targets: [
        .binaryTarget(
            name: "HelloWorldFramework",
            // This is relative to the Package.swift file. This should point to the output from
            // "valdi export ios"
            path: "HelloWorldFramework.xcframework"
        ),
    ]
)
```

This file can be added as a local dependency using `File` -> `Add Package Dependencies` -> `Add Local`.
If you have a `Project.swift` file representing your app, you can add a dependency to the framework by adding the path to the `Package.swift` file in the `packages` array, and add a dependency to the package:

```swift
let project = Project(
    name: "MyApp",
    packages: [
        .package(path: "path/to/HelloWorld"),
    ],
    targets: [
        .target(
            dependencies: [
                .package(product: "HelloWorld"),
            ],
        ),
    ]
)
```

You can use `.xcframework` directly and avoid having to create a `Package.swift` file entirely:

```swift
let project = Project(
    name: "MyApp",
    packages: [
    ],
    targets: [
        .target(
            dependencies: [
                .xcframework(path: "path/to/HelloWorld.xcframework"),
            ],
        ),
    ]
)
```

### CocoaPods

CocoaPods does not have great support for local `.xcframework` files. We recommend you add your `.xcframework` into Xcode directly (see Xcode instructions), which will edit the `.xcodeproj` file and leave the CocoaPods's managed `.xcworkspace` alone. If you really want to expose the `.xcframework` as a Pod, please look at this [CocoaPods GitHub thread](https://github.com/CocoaPods/CocoaPods/issues/10288). Some people there have reported having some successes with getting it work.

## Android

You can export your project as a `aar` library for Android using the `valdi export android` command:

```sh
valdi export android --output_path my_library.aar
```

This will build your Valdi modules defined from the `valdi_exported_library()` target, as well as the Valdi runtime, and package it into an Android library that works for x86_64, arm64 and armv7 architectures.
Note that by default, the library is compiled in debug module. You use the `--build_config release` to build in release mode:

```sh
valdi export android --build_config release --output_path my_library.aar
```

### Gradle

You can add a dependency to your built `.aar` using the `implementation files()` in the `dependencies` section as follow:

```groovy
dependencies {
    implementation files('libs/my_library.aar')
    // Rest of your deps...
}
```

This assumes that the output of `valdi export android` is set to `libs/my_library.aar`, relative to the `build.gradle` file.
Please note that the built `.aar` file might embed some dependencies that you are already using in your project. If you are seeing build errors with duplicated classes between your project and what is inside the `.aar`, you can use the `android_excluded_class_path_patterns` attribute to exclude some specific class paths.
For example if your project already uses AndroidX lifecycle, you might want to it strip out:
```python
valdi_exported_library(
    name = "hello_world_export",
    android_excluded_class_path_patterns = [
        'androidx/lifecycle/.*',
    ],
)
```
`valdi_exported_library()` by default strips out Kotlin runtime files, so it expects that the project importing the `.aar` already bundles Kotlin.

You should now be able to import the Valdi runtime classes and instantiate your component:

```kotlin
import com.snap.valdi.ValdiRuntimeManager
import com.snap.valdi.support.SupportValdiRuntimeManager
import com.snap.valdi.modules.hello_world.App

class MainActivity : Activity() {
    var runtimeManager: ValdiRuntimeManager? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // This needs to execute at least once
        // The name of the library will match the name of
        // your valdi_exported_library() target.
        System.loadLibrary("hello_world_export")

        // For best performance, your app should maintain a singleton
        // of the Valdi runtime manager.
        val runtimeManager = SupportValdiRuntimeManager.createWithSupportLibs(this.applicationContext)
        this.runtimeManager = runtimeManager

        setContentView(App.create(runtimeManager.mainRuntime))
    }
}
```
