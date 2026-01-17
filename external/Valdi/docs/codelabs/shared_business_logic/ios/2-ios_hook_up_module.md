# Hook up the module
Now that we have the module bundled up and the code in the right place, we can start using our calculator.

## Set up for the calculator

*TODO*: Use pubicly available source code for intergration point.

We're going to leave the playground setup as is and create our calculator alongside it. The calculator doesn't have any UI so we will add logging to understand what's going on.

In `SCValdiPlaygroundViewController.m` (**TODO**: path to pubicly available source), import the native functions.

```objectivec
#import <SCCHelloWorld/SCCHelloWorldCalculatorToString.h>
#import <SCCHelloWorld/SCCHelloWorldCreateCalculator.h>
#import <SCCHelloWorld/SCTotesIsMagic.h>
```

If Xcode is unhappy about these imports, recompile.

We need to hold onto a reference to the `SCValdiRuntime` so add a local variable to keep track of it.

```objectivec
@implementation SCValdiPlaygroundViewController {
    SCCPlaygroundView *_contentView;
    id<SCValdiRuntimeProtocol> _runtime;
}
```

Then inside of `initWithValdiServices`, intialize the `_runtime`.

```objectivec
if (self = [super init]) {
    ...
        
    _runtime = valdiServices.valdiRuntimeProvider.target.runtime;
}
```

Now let's create a function where we will create and use our calculator.

```objectivec
- (void)calculate:(id<SCValdiJSRuntime>)runtime
{
    NSLog(@"Calculator setup!");
}
```

This is just a stub for now, we'll need the `SCValdiJSRuntime` soon.

And then in `viewDidLoad`, we'll call the new function.

```objectivec
- (void)viewDidLoad
{
    [self.view addSubview:_contentView];    
    [self calculate:_runtime.jsRuntime];
}

```

When you load up the Playground, your function will be called. Check out the logs in XCode and make sure you can find the `Calculator setup!` message.

## JavaScript execution primer

Before we get started, let's walk through how JavaScript function execution works with Valdi.

Function calls are always executed on the JavaScript thread. Valdi abstracts all of that away for you so you don't have to worry about it but it's helpful to understand what's going on.

If you need to get the result of your function back "synchronously" for use in native code, you can. Valdi will dispatch the work to run on the JS thread and then wait for the result before continuing on.

If async execution is fine, you can use the `getJSRuntimeWithBlock` API.

## Calculate "synchronously"
Now that you have the basics setup, let's actually use our typescript functions.

Inside the `calculate` function, create a reference to the typescript `createCalculator` helper function.

```objectivec
SCCHelloWorldCreateCalculator *createCalculatorFn = [SCCHelloWorldCreateCalculator functionWithJSRuntime:runtime];
```

Then we can call our function and get a reference to the calculator.

```objectivec
id<SCCHelloWorldICalculator> calculator = [createCalculatorFn createCalculatorWithStartingValue:1];
```

Do some math and then log the result.

```objectivec
[calculator addWithValue:2];
[calculator mulWithValue:4];

NSLog(@"Calculator value after ((1 + 2) * 4): %f", [calculator total]);
```

Stop at this point, recompile the app and navigate to the Playground. Make sure you see the output of your logging in Xcode.

Let's play with that other utility function.

```objectivec
SCCHelloWorldCalculatorToString *calculatorToString = [SCCHelloWorldCalculatorToString functionWithJSRuntime:runtime];
    
NSLog(@"TypeScript calculator to string: %@", [calculatorToString calculatorToStringWithCalculator:calculator]);
```

Recompile and check your logs.

## Get the magic asynchronously
If you don't need your functions to return a value synchronously to platform code, you can execute them asynchronously.

Let's execute the magic function with `getJSRuntimeWithBlock` inside of `viewDidLoad`

```objectivec
[_runtime getJSRuntimeWithBlock:^(id<SCValdiJSRuntime> runtime) {
    SCTotesIsMagic *magicFn = [SCTotesIsMagic functionWithJSRuntime:runtime];
    NSLog(@"Result is: %@", [magicFn showMeTheMagic]);
}];
    
```

Rebuild iOS, navigate to the playground, and check your logs.

### [Check out the Android version >](../android/1-android_setup_for_development.md)
