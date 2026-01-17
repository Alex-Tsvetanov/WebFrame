# RxJS

## Introduction

[RxJS](https://rxjs.dev/) is a widely used in the industry library to work with async code use reactive paradigm.

From the official website:

> RxJS is a library for composing asynchronous and event-based programs by using observable sequences.

RxJS is used extensively at Snap and supported by Valdi. RxJS library is available under `valdi_rxjs` module.

## Linters

To further enforce some of the best practices, Valdi enables the following RxJS linter rules (click the URL to see the up to date linter description):

### [rxjs/no-sharereplay](https://github.com/cartant/eslint-plugin-rxjs/blob/main/docs/rules/no-sharereplay.md)

### [rxjs/no-async-subscribe](https://github.com/cartant/eslint-plugin-rxjs/blob/main/docs/rules/no-async-subscribe.md)

### [rxjs/no-ignored-observable](https://github.com/cartant/eslint-plugin-rxjs/blob/main/docs/rules/no-ignored-observable.md)

### [rxjs/no-ignored-subscription](https://github.com/cartant/eslint-plugin-rxjs/blob/main/docs/rules/no-ignored-subscription.md)

To avoid memory leaks and unexpected crashes all created subscriptions must be unsubscribed using the `unsubscribe` call or using one of the helpers below.

There are two ways to ease to manage subscriptions in Valdi:

1. Use [Subscription#add](https://rxjs.dev/api/index/class/Subscription#add) method and invoke [Subscription#unsubscribe](https://rxjs.dev/api/index/class/Subscription#unsubscribe) inside `onDestroy`
2. Use [Component#registerDisposable](./core-component.md#lifecycle) method which would invoke `unsubscribe` method for you during `onDestroy`

#### Use Component#registerDisposable

Use `Component#registerDisposable` to automatically call `unsubscribe`/`dispose` during `onDestroy` for created subscriptions/disposables if your class is a child of `Component`:
```tsx
import { Component } from 'valdi_core/src/Component';
import { Observable } from 'valdi_rxjs/src/Observable';

export class MyComponent extends Component {
 private myObservable: Observable<boolean> = new Observable<boolean>();
 onCreate() {
   const sub = this.myObservable.subscribe(data => {
     // Handle data
   });
   this.registerDisposable(sub);
 }
}
```

#### Use [Subscription#add](https://rxjs.dev/api/index/class/Subscription#add)

Use `Subscription#add` to keep track of created subscription and manually control when `unsubscribe` method is called:

```tsx
import { Subscription } from 'valdi_rxjs/src/Subscription';

export class MyComponent {
 private subscription: Subscription = new Subscription();
 constructor() {
   const sub = this.myObservable.subscribe(data => {
     // Handle data
   });
   this.subscription.add(sub);
 }

 cleanup() {
   this.subscription.unsubscribe();
 }
}
```

### [rxjs/no-nested-subscribe](https://github.com/cartant/eslint-plugin-rxjs/blob/main/docs/rules/no-nested-subscribe.md)

### [rxjs/no-unbound-methods](https://github.com/cartant/eslint-plugin-rxjs/blob/main/docs/rules/no-unbound-methods.md)
