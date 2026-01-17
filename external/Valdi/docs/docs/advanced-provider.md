# Provider

## Introduction

Provider API provides a way to pass dependencies from parent component to children without manually declaring them
at each level of component tree.

This API is very similar to [React Context](https://reactjs.org/docs/context.html) and others examples like:
 - [Flutter Provider](https://pub.dev/documentation/provider)
 - [Swift Environment Object](https://developer.apple.com/documentation/swiftui/environmentobject)
 - [Android Composition Local](https://developer.android.com/jetpack/compose/compositionlocal)

This API should be useful when you need to pass the instance of your service or any other readonly data into components tree.

You may notice that this API has similarity with Valdi Component Context. 

Provider doesn't have any platform related logic and focused on work in TypeScript platform only.

>[!NOTE]
>This API could not be helpful for passing data which you plan to change frequently in the application.
>
> View Model mechanism still should be a source of truth for such updates. 
>
> Provider API is designed for provide an ability to build share mechanism for sharing data, not store data itself.


### Using Provider API
For example, we have `MyService` and we would like to pass it through the components tree
```typescript jsx
class MyService {
  public sayHello() {
    return 'sayHello';
  }
}
```

To create a new Provider you need to call `createProviderComponent` with a unique provider name and specify the type of needed dependency using the generic type parameter.

```typescript jsx
const MyServiceProvider = createProviderComponentWithKeyName<MyService>('MyServiceProvider');
```

`createProviderComponent` returns a special Provider component which you can use within your component tree. 
When you use the returned Provider component, you need to pass the initial `value` that it exposes to any component within its subtree of the component tree. The type of `value` has to be compatible with the type you specified when calling `createProviderComponent`.

```typescript jsx
class AppRoot extends Component {
    private readonly myService = new MyService(); // the service could also have been injected by the native component context

    onRender(): void {
        <MyServiceProvider value={this.myService}>
            <App />
        </MyServiceProvider>;
    }
}
```

>[!NOTE]
> If you decide to change the value of your Provider, it will trigger a rerender for all of the children that you have inside Provider Component.
>
>In our example it will be `<App />`.


After initialization step you can consume `Provider.value` in any place under the `MyServiceProvider` component tree.

To get the instance of our `MyService` somewhere in children we need at first 
to extend an original view model of children component from `ProvidersValuesViewModel` interface.

```typescript jsx
interface MyConsumerComponentViewModel extends ProvidersValuesViewModel<[MyService]> {
    myUnrelatedOtherProperty: boolean;
}
```

In this example we're planning to add the Provider value to an existing component which
already has a view model property `myUnrelatedOtherProperty: boolean`;

After extending component view model, we can consume provider values from the view model. 
`ProvidersValuesViewModel` interface adds a special property which is called `providersValues`
and contains an array of all resolved providers values from parent providers.

```typescript jsx
class MyConsumerComponent extends Component<MyConsumerComponentViewModel> {
    onRender() {
        const [myService] = this.viewModel.providersValues;
        <label value={myService.sayHello()} />;
    }
}
```

In our example we expect to consume only `MyServiceProvider`. 
To learn how to consume multiple providers, please read below. 

The last step to make this example work we need to define which Provider and component 
we would like to use. For this step we need to use `withProviders` function. We need to pass 
into this function the Provider component which we created before at step one. 

```typescript jsx
const createComponentWithMyServiceProvider = withProviders(MyServiceProvider)
```

`withProviders` function returns a function which allows you to inject the providers values
into your component without modification. 

```typescript jsx
const MyConsumerComponentWithMyService = createComponentWithMyServiceProvider(MyConsumerComponent)
```

This pattern is called [Higher Order Component](https://blog.jakoblind.no/simple-explanation-of-higher-order-components-hoc/) which can be familiar for web devs. 

To simplify our last steps we can do the following which more nature
```typescript jsx
const MyNewComponent = withProviders(
    MyServiceProvider
)(MyConsumerComponent);
```

and then finally we can use our new component with injected dependency in the app

```typescript jsx
class App extends Component {
    onRender(): void {
        <view>
            <MyNewComponent myUnrelatedOtherProperty={true} />
        </view>;
    }
}
```

As you may notice the `<App />` component doesn't know anything about `MyService` which this is the key point of usage Provider API.

In this case intermediate components don't know about needed dependencies, and they can be passed implicitly.

If you need to consumer several providers, you can use the same API.

```typescript jsx
const MyConsumerComponentWithProviderValue = withProviders(
    Provider1,
    Provider2,
    Provider3
)(MyConsumerComponent);
```

As a result, you can get all resolved values the same view model from array `providersValues`.
Please note, that this array is strong typed and used typescript feature which is called a [tuple](https://www.typescriptlang.org/docs/handbook/2/objects.html#tuple-types).

```typescript jsx
class MyConsumerComponent extends Component<MyConsumerComponentViewModel> {
    onRender() {
        // 
        const [providerValue1, providerValue2, providerValue3] = this.viewModel.providersValues;
        ...
    }
}
```

The full implementation can be viewed [here](../../src/valdi_modules/src/valdi/valdi_core/src/provider).
