# Implement recommendFriendsCallback
It is time to actually do the thing we came here to do!
Lets replace our `dummyRecommendFriendsCallback` callback with a real callback:

```swift
let friendsCallback: ((@escaping ([SCCHelloWorldFriend]) throws -> Void) throws -> String) = { completion in
    snapchatterServices.snapchattersDataFetcher.target()?.suggestedSnapchatters(for: .addFriends, completionQueue: DispatchQueue.main) { snapchatters, error in
        var friends = [SCCHelloWorldFriend]()
        if error == nil, let snapchatters = snapchatters, !snapchatters.isEmpty {
            for snapchatter in snapchatters {
                guard let displayName = snapchatter.displayName else { continue }
                let friend = SCCHelloWorldFriend(name: displayName)
                friends.append(friend)
            }
        }
        do {
            try completion(friends)
        } catch {
            // Handle error
        }
    }
    return "Success"
}
```

Then use this as the `recommendFriendsCallback` when creating `SCCHelloWorldContext`:
```swift
let context = SCCHelloWorldContext(title: "HelloWorld", recommendFriendsCallback: friendsCallback)
```

That's it. That's the last step.

We request some suggested friends from the **snapchattersDataFetcher** and then when we get them, we iterate through, format the friends into **Friends**, and put them in an array. Then call the completion with the list.

Rebuild your app, load up the page, make some friends.

### [Next >](https://github.com/Snapchat/Valdi/blob/main/docs/codelabs/integration_with_native/ios/4-ios_recommendfriendscallback.md)
