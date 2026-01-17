# Hook up the module
Now that we have the settings row, lets hook up our module to the UI.

## Create ValdiSwiftDemoViewController.swift

Create the file `ValdiSwiftDemoViewController.swift` (`Features/Valdi/EntryPoints/ValdiSettingsSwiftEntryPoint/Sources/ValdiSwiftDemoViewController.swift`)
and add the following to it:
```swift
import valdi_core
import SCValdiServices
import SCCHelloWorld
import UIKit

enum ValdiSwiftDemoViewControllerError: Error {
    case runtimeUnavailable
}

class ValdiSwiftDemoViewController: UIViewController {
    init(valdiServices: SCValdiServices) throws {
        super.init(nibName: nil, bundle: nil)
    }

    override func viewDidLoad() {
    }

    override func viewWillLayoutSubviews() {
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) is not a valid initializer for this view controller")
    }
}
```


## Instantiating the Valdi view

First we want to create a variable to hold our Valdi view. The name of the type of the valdi view is defined in the .tsx file with the `@ExportModule` annotation
Add the following property to the class:

```swift
private let contentView: SCCHelloWorldView
```

Now let's initialize the view in the `init()` function. Insert all of the following before the super.init() call, since contentView needs to be initialized before it's called.

First we want to make sure we have a `runtime` which we need to create our view.
```swift
guard let valdiRuntime = valdiServices.valdiRuntimeProvider.target()?.runtime else {
    throw ValdiSwiftDemoViewControllerError.runtimeUnavailable
}
```

Then we create our ViewModel and Context. Any non-optional parameters must be provided in the constructor on the object.
```swift
let viewModel = SCCHelloWorldViewModel()
viewModel.subtitle = "MySubtitle"

let dummyRecommendFriendsCallback: ((@escaping ([SCCHelloWorldFriend]) throws -> Void) throws -> String) = { completion in
    // Dummy implementation of the callback
    let dummyFriends = [SCCHelloWorldFriend(name: "John Doe"), SCCHelloWorldFriend(name: "Jane Doe")]
    try completion(dummyFriends)
    return "Callback executed"
}
let context = SCCHelloWorldContext(title: "HelloWorld", recommendFriendsCallback: dummyRecommendFriendsCallback)
```

And finally let's create our view
```swift
self.contentView = try SCCHelloWorldView(viewModel: viewModel, context: context, runtime: valdiRuntime)
```

## Hook up Valdi View

In the `viewDidLoad` function add the following to load our Valdi view as a subview:
```swift
contentView.valdiContext.traitCollection = traitCollection
view.addSubview(contentView)
```

And in `viewWillLayoutSubviews`, add the following:
```swift
contentView.frame = view.bounds
```

## Load our view controller when Settings row is clicked

Finally, we want to load our ViewController when the Settings row is clicked.
Back in
`ValdiSwiftDemoViewController.swift` (`Features/Valdi/EntryPoints/ValdiSettingsSwiftEntryPoint/Sources/ValdiSwiftDemoViewController.swift`) add the following in `onSettingsRowHandleContext` in the `do` block:

```swift
let settingsPageViewController = try ValdiSwiftDemoViewController(
    valdiServices: valdiServices
)
navigationController.pushViewController(settingsPageViewController, animated: true)
```

Now rebuild iOS, navigate to the settings, and load up your new UI.

### [Next >](https://github.com/Snapchat/Valdi/blob/main/docs/codelabs/integration_with_native/ios/3-ios_hook_up_snapchatter_service.md)
