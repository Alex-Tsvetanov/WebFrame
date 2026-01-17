# Component Context

## Basics
The context object can be used to provide services, static immutable data and bridge methods to all Components in your Component tree.
It can be accessed through `this.context` inside your Component. It is initially created from native code and passed to the root Component, which automatically forwards it by default to all children:

TypeScript:
```ts
// @Context
// @ExportModel({ios: 'SCMyComponentContext'})
interface Context {
  serverURL: string;
}

// @Component
// @ExportModel({ios: 'SCMyComponentView'})
class MyComponent extends Component<any, Context> {
  onCreate() {
    // Will print http://api.server.com in the console
    console.log(this.context.serverURL);
  }
}
```

Native code:
```objectivec
// Instantiate the context data structure
SCMyComponentContext *context = [[SCMyComponentContext alloc]
    initWithServerURL:@"http://api.server.com"];
// Instantiate the view with the context passed as parameter
SCMyComponentView *view = [[SCMyComponentView alloc]
    initWithViewModel:viewModel
     componentContext:context];
```

If one of your child uses a different Context type, you can translate it to a type it understands through the `context` attribute:
```tsx
import { ChildComponent, ChildContext } from './ChildComponent';

interface MyContext {
  currentUser: User;
}

class MyComponent extends Component<object, Context> {

  onRender(): void {
    <layout>
      <ChildComponent context={this.getChildComponentContext()}/>
    </layout>
  }

  getChildComponentContext(): ChildContext {
    return {user: this.context.currentUser};
  }
}
```

## Context or ViewModel?

Unlike ViewModel, the Context object cannot be changed once it's set. It should therefore not impact the rendering of your tree. Context is the best place to provide services, may they be implemented by native code or TypeScript. We recommend using the Context object for your root Component only, as a way to provide native services to TypeScript.

## Factories

Valdi objects do not leverage dependency injection and objects (whether that be contexts, view models, etc.) are instantiated with regular constructors, not inject constructors, which requires more boilerplate to write.
In order to inject certain parameters, the Valdi compiler now generates two different native classes: the  class that it was emitting before and a factory with a create function that returns that first class. The factory will leverage our existing native dependency graphs.

All injected parameters will be passed into the factory’s constructor, requiring that they be provided by the native DI graph. All non injected parameters will be parameters in the create function.


TypeScript:
```ts
// @Context
// @ExportModel({android: 'com.snap.mymodule.MyDataSyncerContext'})
interface MyDataSyncerContext {
   // @Injectable 
   configurationProvider: ConfigurationProvider,
   key: string;
}
```

Native:
```kotlin
class MyDataSyncerContextFactory @Inject(
    val configurationProvider: ConfigurationProvider
) {
   fun create(key: String): MyDataSyncerContext {
       return MyDataSyncerContext(configurationProvider, key)
   }
}
```

To use Factories, you will need to make changes in both the client repo and in native.

In the client repo:
* Mark any parameters on the interface that you want to be provided from the dagger graph with an `@Injectable` annotation. If you want the parameter to only be injected in one client, you have the option of passing `androidOnly: true` or `iOSOnly: true` to the annotation to indicate that.

<!-- TODO: is DI in scope for Open Source? -->

[contributes-valdi-context]: #todo-link-to-source

In native:
* Wherever you need access to the Valdi object, inject the factory. Instead of manually instantiating the object with all the dependencies, call the factory’s create method with the non-injectable parameters.
* On Android, you will be responsible for telling Dagger how to provide the injected parameters. You can use the Anvil plugin [`@ContributesValdiContext`][contributes-valdi-context] to auto-generate some of the boilerplate.

```kotlin
class MyAndroidClass @Inject(
    private val myDataSynceContextFactory: MyDataSyncerContextFactory
) {
   fun doSomething() {
    ...
    val key = "myKey"
    val context = myDataSyncerContextFactory.create(myKey)
    ...
   }
}
```