# Testing
Remember when we added all of those ids? It's time to put them to use. 

In this section we'll be working entirely within the Android code base.

Because Valdi is it's own special rendering engine, we won't be able to create Robolectric tests, we need to load up the whole runtime so these are going to be [instrumentation tests](https://source.android.com/docs/core/tests/development/instrumentation).

On Android, Valdi may render your UI with native Android Views or with SnapDrawing. Android Views are nice and stable and well known, SnapDrawing is the new hotness that's fast but still in A/B.

You don't have to worry about the implementation details though because Valdi has a library that makes all Valdi UI compatible with [Espresso](https://developer.android.com/training/testing/espresso/basics). 

## Compspresso mappings

**TODO**: Update links when source is moved to publicly available locations.

The full libraries are available [here](#todo) but let's go through how the Espresso functions that you know and love map to the Valdi versions.

| Espresso | Compspresso | Description |
| --- | --- | --- |
| `onView` | `onValdiElement` | Find a view so you can do things with it. |
| `withId` | `withAccessibilityId` | Identify a view. |

That's it. The rest of it is going to behave as expected. 

### Overlap
If you are testing traditional Android views in the same tests as Valdi views, you're going to have trouble with import overlap. Conveniently, Kotlin allows you to [alias](https://kotlinlang.org/docs/packages.html#imports) imports to get around this problem.

## Set up the test files
Setup is the most painful part of this, after that, the testing is easy.

We need a few files to get started.

Create the files.

`mkdir -p components/valdi/settings/core/src/androidTest/java/com/snap/valdi/settings/core/`

`touch components/valdi/settings/core/src/androidTest/java/com/snap/valdi/settings/core/HelloWorldTests.kt`

`touch components/valdi/settings/core/src/androidTest/BUILD.bazel`

`touch components/valdi/settings/core/src/androidTest/AndroidManifest.xml`

Integration tests conventionally go in a parallel `androidTest` directory so that's what we're doing here. 

In `HelloWorldTests.kt`, create a bare bones test.

```kotlin
package com.snap.valdi.settings.core

import androidx.test.runner.AndroidJUnit4
import org.assertj.core.api.Assertions.assertThat
import org.junit.runner.RunWith
import org.junit.Test

@RunWith(AndroidJUnit4::class)
class HelloWorldTests {

    @Test
    fun dummy() {
        assertThat("things").isEqualTo("things")
    }
}
```

We also need a barebones `AndroidManifest.xml`

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.snap.valdi.sample.test"
    >

    <uses-sdk android:minSdkVersion="19"
        android:targetSdkVersion="30"/>

    <instrumentation
        android:name="com.snap.test.instrumentation.runner.SnapAndroidJUnitRunner"
        android:targetPackage="com.snap.valdi.sample" />

</manifest>
```

The `BUILD` file is where it gets complicated.

Add the imports at the top.

```bazel
load(
    "//bzl:sdk_variables.bzl",
    "SAMPLE_MIN_SDK_VERSION",
    "SAMPLE_TARGET_SDK_VERSION",
)
load("//bzl/instrumentation:build_instrumentation_test.bzl", "instrumentation_test_library")
load("//bzl/instrumentation/spoon_instrumentation_rule:defs.bzl", "spoon_instrumentation_test")
```

Then we setup the test library.

```bazel
instrumentation_test_library(
    name = "valdi-playground-test-lib",
    srcs = glob([
        "java/**/*.java",
        "java/**/*.kt",
    ]),
    manifest = "AndroidManifest.xml",
    deps = [
        "//platform/valdi/core",
        "//platform/valdi/test-support:lib",
        "//platform/test-attribution:lib",
        "//platform/test-instrumentation-support:lib",
        "@valdi_modules//:hello_world",
        "@test_mvn//:androidx_test_espresso_espresso_core",
        "@test_mvn//:androidx_test_runner",
        "@test_mvn//:junit_junit",
        "@test_mvn//:org_assertj_assertj_core",
        "@test_mvn//:org_hamcrest_hamcrest_core",
        "@test_mvn//:org_hamcrest_hamcrest_library",
    ],
)
```

This is where we pull in our test files and their dependencies.

Then we build our lib into an Android test binary.

```bazel
android_binary(
    name = "valdi-playground-test-app",
    testonly = True,
    debug_key = "//scripts/keystore:andy-keystore",
    instruments = "//platform/valdi/sample:valdi-sample-apk",
    manifest = "AndroidManifest.xml",
    manifest_values = {
        "minSdkVersion": SAMPLE_MIN_SDK_VERSION,
        "applicationId": "com.snap.valdi.sample.test",
        "targetSdkVersion": SAMPLE_TARGET_SDK_VERSION,
    },
    tags = ["no-remote-cache"],
    deps = [":valdi-playground-test-lib"],
)
```

Here we're piggybacking off of the `valdi-sample-apk` because we don't have a sample app specifically for the playground.

Only now can we setup the integration tests.

```bazel
spoon_instrumentation_test(
    name = "valdi-playground-instrumentation-test",
    # Size must be medium to accommodate for 60s average test execution time
    # (avoids risk of exceeding the limit by a hair)
    size = "medium",
    test_app = ":valdi-playground-test-app",
)
```

This is the bare minimum setup to run some tests on a Valdi UI.

At this point, stop and make sure everything works.

`./bazelw test //components/valdi/settings/core/src/androidTest:valdi-playground-instrumentation-test`

You should see one passing test.

## Interlude: real world testing
In this codelab, we're going to test an individual Component. It's unlikely that you'll do this in the real world but the concepts around accessing Valdi components and interacting with them will be valuable when testing UIs that contain both traditional and Valdi UI.

## Test setup
We're going to use the ValdiViewTestRule to create our component and make sure it behaves the way we expect it to. 

```kotlin
@Rule
@JvmField
val viewTestRule = ValdiViewTestRule {
    val context = HelloWorldContext(
        title = "Hello world",
        recommendFriendsCallback = {
            "success"
        }
    )
    HelloWorldView.create(it, HelloWorldViewModel(), context)
}
```

We need to create a context with some defaults but we can use the ViewModel as is. 

Then we need to handle startup and teardown.

```kotlin
@Before
fun prepare() {
    viewTestRule.waitForNextRender()
}

@After
fun tearDown() {
    viewTestRule.destroyView()
}
```

We are `waitForNextRender`ing to make sure that the component is displayed before we start trying to test it.

## First test
Let's double check that the UI has rendered. We have one element that we know is going to render no matter what.

```kotlin
@Test
fun rendersRecommendFriendsButton() {
    onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommend_friends_button))
        .check(matchesAnyDescendant(withValdiAttribute("value", "Recommend friends")))
}
```

Here we're using the Valdispresso `onValdiElement` to access the UI. Then we're using our id, full qualified here as `com.snap.valdi.modules.R.id.hello_world__recommend_friends_button` to find the button.

`recommend_friends_button` refers to a `CoreButton` but we know that to display the text, it has to contain a `<label>` with a `value` that is "Recommend friends". So we `check` that there is some descendant `withValdiAttribute`.

Run your tests and make sure they pass.

`./bazelw test //components/valdi/settings/core/src/androidTest:valdi-playground-instrumentation-test`

## Updating the ViewModel
This is something that we didn't do in our UI but we can still test the behavior any way.

If you update the `ViewModel` from native, it has the same effect as if you update the component's parameters from TypeScript.

Let's make sure that the starting value is what we think it is. 

```kotlin 
fun resetSubtitleThroughViewModel() {
    onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__subtitle))
        .check(matches(withValdiAttribute("value", null)))
}
```

We never set the value of the **subtitle** in the `ViewModel` on setup so it should be null.

Now let's give the view a new `ViewModel`. In the same function:

```kotlin
var helloWorldViewModel = HelloWorldViewModel()
helloWorldViewModel.subtitle = "subtitle"

viewTestRule.view.viewModel = helloWorldViewModel

viewTestRule.waitForNextRender()
```

We need to `waitForNextRender` because the values won't update until the next render.

Now we can check the new value.

```kotlin
onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__subtitle))
    .check(matches(withValdiAttribute("value", "subtitle")))
```

Let's try this one more time to make sure it really works.

```kotlin
viewTestRule.view.viewModel = helloWorldViewModel
helloWorldViewModel.subtitle = "new subtitle"
viewTestRule.waitForNextRender()

onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__subtitle))
    .check(matches(withValdiAttribute("value", "new subtitle")))
```

Run your tests and make sure they pass.

## Test the `recommendFriendsCallback`
Let's make sure that the UI responds correctly when you tap on the **Recommend friends** button.

First lets return some friends from the callback in the `ValdiViewTestRule`.

```kotlin
val context = HelloWorldContext(
    title = "Hello world test",
    recommendFriendsCallback = {
        it(listOf(HelloWorldFriend("Steve Rogers"),
                    HelloWorldFriend("Tony Stark"),
                    HelloWorldFriend("Thor Odinson")))
        "success"
    }
)
```

Then we need to interact with the with button that triggers the callback.

```kotlin
@Test
fun recommendsFriends() {
    onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommend_friends_button))
        .perform(click())

    viewTestRule.waitForNextRender()
}
```

**TODO**: Update link when source is publicy available.
You can find the full set of interactions in [ValdiElementActions](#todo).

Again, we're waiting for the next render so that the UI will update.

Then we can check and make sure our friends are displayed as expected.

```kotlin
onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommended_friends_container))
    .check(matchesAnyDescendant(withValdiAttribute("value", "Steve Rogers")))

onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommended_friends_container))
    .check(matchesAnyDescendant(withValdiAttribute("value", "Tony Stark")))

onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommended_friends_container))
    .check(matchesAnyDescendant(withValdiAttribute("value", "Thor Odinson")))
```

Make sure your tests are all passing.

`./bazelw test //components/valdi/settings/core/src/androidTest:valdi-playground-instrumentation-test`

## Full solution
Here's the full test file if you need it.

```kotlin
package com.snap.valdi.settings.core

import androidx.test.runner.AndroidJUnit4
import com.snap.valdi.test.ValdiViewTestRule
import com.snap.playground.HelloWorldContext
import com.snap.playground.HelloWorldFriend
import com.snap.valdi.test.ValdiElementActions.click
import com.snap.valdi.test.ValdiElementAssertions.matchesAnyDescendant
import com.snap.valdi.test.ValdiElementMatchers.withAccessibilityId
import com.snap.valdi.test.ValdiElementAssertions.matches
import com.snap.valdi.test.ValdiElementMatchers.withValdiAttribute
import com.snap.valdi.test.ValdiEspresso.onValdiElement
import com.snap.playground.HelloWorldView
import com.snap.playground.HelloWorldViewModel
import org.assertj.core.api.Assertions.assertThat
import org.junit.runner.RunWith
import org.junit.Test
import org.junit.Rule
import org.junit.After
import org.junit.Before


@RunWith(AndroidJUnit4::class)
class HelloWorldTests {

    @Rule
    @JvmField
    val viewTestRule = ValdiViewTestRule {
        val context = HelloWorldContext(
            title = "Hello world test",
            recommendFriendsCallback = {
                it(listOf(HelloWorldFriend("Steve Rogers"),
                          HelloWorldFriend("Tony Stark"),
                          HelloWorldFriend("Thor Odinson")))
                "success"
            }
        )
        HelloWorldView.create(it, HelloWorldViewModel(), context)
    }

    @Before
    fun prepare() {
        viewTestRule.waitForNextRender()
    }

    @After
    fun tearDown() {
        viewTestRule.destroyView()
    }

    @Test
    fun dummy() {
        assertThat("things").isEqualTo("things")
    }

    @Test
    fun rendersRecommendFriendsButton() {
        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommend_friends_button))
            .check(matchesAnyDescendant(withValdiAttribute("value", "Recommend friends")))
    }

    @Test
    fun contextSetsTitle() {
        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__title))
            .check(matches(withValdiAttribute("value", "Hello world test")))
    }

    @Test
    fun resetSubtitleThroughViewModel() {
        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__subtitle))
            .check(matches(withValdiAttribute("value", null)))

        var helloWorldViewModel = HelloWorldViewModel()
        helloWorldViewModel.subtitle = "subtitle"

        viewTestRule.view.viewModel = helloWorldViewModel

        viewTestRule.waitForNextRender()

        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__subtitle))
            .check(matches(withValdiAttribute("value", "subtitle")))

        viewTestRule.view.viewModel = helloWorldViewModel
        helloWorldViewModel.subtitle = "new subtitle"
        viewTestRule.waitForNextRender()

        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__subtitle))
            .check(matches(withValdiAttribute("value", "new subtitle")))
    }

    @Test
    fun recommendsFriends() {
        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommend_friends_button))
            .perform(click())

        viewTestRule.waitForNextRender()

        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommended_friends_container))
            .check(matchesAnyDescendant(withValdiAttribute("value", "Steve Rogers")))

        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommended_friends_container))
            .check(matchesAnyDescendant(withValdiAttribute("value", "Tony Stark")))

        onValdiElement(withAccessibilityId(com.snap.valdi.modules.R.id.hello_world__recommended_friends_container))
            .check(matchesAnyDescendant(withValdiAttribute("value", "Thor Odinson")))
    }
}
```

### [Check out the iOS version >](https://github.com/Snapchat/Valdi/blob/main/docs/codelabs/integration_with_native/ios/1-ios_setup_for_development.md)