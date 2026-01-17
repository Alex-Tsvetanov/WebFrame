# Valdi for React Developers

Valdi and React are both inspired by Functional Reactive Programming UI patterns.

The "state" of the application is fed into root of the render tree, and components use this state to produce rendered elements. Events emitted by the user interface are then used to change the state of the application in order to produce an updated render tree.

A process compares the new output to the old output and updates the runtime to change the running application.

## Components: Valdi vs React

In React there are two flavors of components: `function` components and `class` components.

Valdi does not have an API equivalent to React’s `function` components.

### React `class` Components

React’s _`class` components_ inherit from `class React.Component<P, S>` and _at least_ implement an `onRender(): JSX.Element`

Props (`P`) are stored as a property (`this.props`) of the `Component` instance and and are provided by the parent of the component via JSX attribute syntax:

```tsx
<Heading title="Hello World" />
```

A component is rendered when:

- It is given new props
- It calls `this.setState({...})` with new state

### Valdi’s `class Component<ViewModel, State>`

Valdi provides a `Component<ViewModel, State, Context>` class similar to React’s `React.Component<P, S>` with the same runtime characteristics for rendering:

- Changes to a component’s props from a parent component will trigger a render pass
- Calling `this.setState({...})` with changed state values will trigger a render pass

In Valdi the concept of `Props` are called `ViewModel`s but are otherwise treated identically. In both Valdi and React, the internal/private set of properties that can trigger a render are called `State`.

> **See also:** [The Mighty Component](./core-component.md) for complete documentation on Valdi components and [Component States](./core-states.md) for state management details.

#### Example React Component ported to Valdi

The following is a trivial React `Component` that uses `Props` and `State`.

```tsx
import { Component } from "react";

interface Props {
  label: string;
  onDoThing: (count: number) => void;
}

interface State {
  counter: number;
}

class MyComponent extends Component<Props, State> {
  state = {
    counter: 0,
  };

  handleTap = () => {
    const counter = this.state.counter + 1;
    this.setState({ counter });
    this.props.onDoThing(counter);
  };

  onRender(): JSX.Element {
    return (
      <view onTap={this.handleTap}>
        <label
          value={`${this.props.label} (click count ${this.state.counter})`}
        />
      </view>
    );
  }
}
```

This component can be used **almost** as is in Valdi:

```tsx
import { StatefulComponent } from "valdi_core/src/Component";

interface ViewModel {
  label: string;
  onDoThing: (count: number) => void;
}

interface State {
  counter: number;
}

class MyComponent extends StatefulComponent<ViewModel, State> {
  state = {
    counter: 0,
  };

  handleTap = () => {
    const counter = this.state.counter + 1;
    this.setState({ counter });
    this.viewModel.onDoThing(counter);
  };

  onRender(): void {
    <view onTap={this.handleTap}>
      <label
        value={$`{this.viewModel.label} (click count ${this.state.counter})`}
      />
    </view>;
  }
}
```

Valdi provides a "stateless" component: `Component` which has no `State` type or `setState` method. Both `Component` and `StatefulComponent` have a third generic parameter: `Context`.

This is not the same as React's `Context` API. It is part of Valdi's native API integration. See _[Component Context](./native-context.md)_ to learn more about Valdi's `Context` API.

### Component `key` in React and Valdi

Each Component can be given a `key` property and it functions the same way as React so the same best practices apply.

If you have a list of items and each item has some intrinsic way of identifying it, using a stable key to reference it will maintain its lifecycle during render passes.

```tsx
    renderListElements() {
        for (const item of STUFF) {
            <ListItem key={item}><label value={item} /></ListItem>
        }
    }
```

## Differences from React

There are some additional differences, like [life-cycle methods](./core-component.md#lifecycle) but the same best practices remain:

- Do not mutate `this.state`, call `this.setState()`. Valdi sets it to `ReadOnly<State>`
- Do not mutate `this.viewModel`, the `viewModel`’s value is defined by the parent. Valdi sets it to `ReadOnly<ViewModel>`
- Inline/lambda functions and non-literal values should be stored as properties of the `class` instance to keep from changing props unintentionally during render. For callback props there is a utility to help with this: [`createReusableCallback`](../../src/valdi_modules/src/valdi/valdi_core/src/utils/Callback.ts#L34)

### Render Output of Valdi vs React.

There is one significant difference to call out between Valdi and React as it pertains to the `onRender()` method and constructing the output tree.

In React, the method is `onRender(): JSX.Element | null`. In Valdi, the method is `onRender(): void`. It returns `void`, nothing is returned from `onRender()`.

Valdi components **do not return rendered elements**.

When `onRender` is called by the Valdi runtime, JSX tags emit the render operations as a side-effect. This means _when_ JSX tags are used is critical to where they end up in the tree.

In React it is common to store a reference to the `JSX.Element` output of a component and then eventually use it in your component’s output.

For example, instead of just allowing a `label: string`, in React it can be specified as `label: JSX.Element | null` and the parent can use any other valid React component.

```tsx
<MyComponent label={<label style={{ color: 'red' }} value="Button label text">}>
```

This cannot be done in Valdi. Not only would this fail to pass TypeScript satic analysis, the `<label>` would be emitted into the render tree at a different location in the tree, not where `MyComponent` uses its `this.viewModel.label` property.

To achieve something similar in Valdi you can use what in the React are sometimes called "render props". It would then be up to `MyComponent` to call the function at the correct time.

```tsx
interface ViewModel {
   label: () => void
}

onRender() {
   <view margin={20}>{this.viewModel.label()}</view>
}
```

Due to the nature of JSX elements being emitted instead of returned, you can also use `for`/`while` loop iterators to generate lists of components.

```tsx
const STUFF = ["beacon", "shovel", "probe", "straps", "harness"];

class SomeComponent extends Component {
  onRender() {
    <List>{this.renderListElements()}</List>;
  }

  renderListElements() {
    for (const item of STUFF) {
      <ListItem>
        <label value={item} />
      </ListItem>;
    }
  }
}
```

In React, this would produce a `<List />` with nothing in it because the `<ListItem />` components are never returned and placed in the component’s output.

```tsx
// Example output of <SomeComponent /> using React
<List />
```

In Valdi, because the `renderListElements()` method is emitting `<ListItem>...</ListItem>` elements after the opening of a `<List>` is emitted, but _before_ the closing `</List>` is emitted, the `<ListItem></ListItem>` elements will be placed inside the `<List />` in the component’s rendered representation:

```tsx
// Example ouptut of <SomeComponent /> using Valdi.
<List>
  <ListItem>
    <label value="beacon" />
  </ListItem>
  <ListItem>
    <label value="shovel" />
  </ListItem>
  <ListItem>
    <label value="probes" />
  </ListItem>
</List>
```

To reiterate, this is because of _when_ the elements were emitted during a render pass which starts when a component’s `onRender` is called and finishes when all leaves are done rendering.

### Component `ref`

Similar to `React`, a `Component`’s `ref` provides a way to access the API of the underlying instance of a child component and interact with it beyond using a `Component`’s `viewModel`.

Ideally the contract between a `Component` and its owner is implemented purely in the component’s `ViewModel` but there are instances where it is less complex to interact with the child’s state with imperative code.

Vald uses the `interface IRenderedElementHolder<T>` and a `ref` is generally assigned to an implementation stored on the instance of the `Component` rendering it.

An example of this is [`ScrollViewHandler`](https://github.com/Snapchat/Valdi_Widgets/blob/main/valdi_modules/widgets/src/components/scroll/ScrollViewHandler.ts#L33) which provides a means of interacting with the view underlying a `<scroll>` component.

This is a trivial example assigns a `ref` to a `<scroll>` native element, and provides a button that scrolls the view down by `10` pixels when pressed.

```tsx

interface ViewModel {
    list: {name: string}[]
}

class SomeComponent<ViewModel> {

    private currentPosition: {x : number, y: number};

    private readonly scrollHandler = new ScrollViewHandler();

    private scrollDownByTen = () => {
        this.scrollHandler.scrollTo(
            this.scrollHandler.scrollX,
            this.scrollHandler.scrollY + 10
        );
    }

    onRender() {
        <layout>
            <view onTap={this.scrollDownByTen}>
                <>
            </view>
            <scroll ref={scrollHandler}>
                {this.viewModel.list.forEach((item) => {
                    <ListItem data={item} />
                })}
            </scroll>
        </layout>
    }
}
```

### Component `children` and the `<slot />` API

In React, a component’s `children` prop is used by a component like any other prop.

In Valdi, a component uses the [`<slot />`](./core-slots.md) API to declare where its children will be rendered.

A Valdi component can specify a different type for its `children` `ViewModel` property. For example the [`MeasuredView` component specifies a function for its children](https://github.com/Snapchat/Valdi_Widgets/blob/main/valdi_modules/widgets/src/components/util/MeasuredView.tsx#L6-L8):

```tsx
export interface MeasuredViewModel extends View {
  children?: (frame: ElementFrame) => void;
}
```

To opt into rendering optimizations, parent componets should use the `$slot` and `$namedSlots` utilities that signal to the rendering system that the children for a component are being changed.

Using `MeasuredView` as an example, the `children` callback is wrapped in `$slot`:

```tsx
import { $slot } from 'valdi_core/src/CompilerIntrinsics';
import { MeasuredView } from 'valdi_widgets/src/components/util/MeasuredView';

onRender() {
    <MeasuredView>
        {$slot(({ widgth, height }) => {
            <view>
            </view>
        })}
    </MeasuredView>
}
```
