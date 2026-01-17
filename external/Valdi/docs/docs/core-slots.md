# The `<slot>`

## Slots
Valdi implements a content distribution API that’s modeled after the current [Web Components spec draft](https://github.com/w3c/webcomponents/blob/gh-pages/proposals/Slots-Proposal.md), using the `<slot>` element to serve as distribution outlets for content.

> Slot provides a component the ability to allow content to be "injected" into its render-tree

Slots, when unused or unspecified, are discarded.

## Slotting contents into another Component

It's often useful to create Components that work as containers for contents that are provided externally. Let's imagine a hypothetical `HorizontalStack` component that takes its children and arranges them on a horizontal line:

```tsx
import { Component } from 'valdi_core/src/Component';

export class HorizontalStack extends Component {
  onRender() {
    <view
      flexDirection='row'
      justifyContent='center'
      padding={20}
      backgroundColor='lightblue'>
      <slot />
    </view>;
  }
}
```

The special `<slot/>` element denotes a placeholder in the UI hierarchy that `HorizontalStack`'s children should be _slotted into_. Here's how you can now use the `HorizontalStack` Component:

```tsx
import { Component } from 'valdi_core/src/Component';
import { HorizontalStack } from 'horizontal_stack/src/HorizontalStack';

export class HelloWorld extends Component {
  onRender() {
    <HorizontalStack>
      <label margin={5} value='red' color='red' backgroundColor='white' />
      <label margin={5} value='green' color='green' backgroundColor='white' />
      <label margin={5} value='blue' color='blue' backgroundColor='white' />
    </HorizontalStack>;
  }
}
```

![Screenshot of the HelloWorld component](./assets/core-slots/IMG_1455.jpg)

## Multiple slots

A Component can have multiple slots. To distinguish between them, a `<slot>` element has a `name` attribute. When a name is not provided, `'default'` is assumed

```tsx
<slot name='good' />
```

At the point of use, the children that are being _slotted into_ the Component should be wrapped in a `<slotted>` element specifying the name of the target `<slot>` in the `slot` attribute. When a `slot` name is not provided, `'default'` is assumed. When 

```tsx
<slotted slot='good'>
  <image width={88} height={88} margin={8} src='https://placecats.com/88/88' />
</slotted>
```

Putting this all together, let's imagine a Component that exposes three slots with different colored backgrounds:

```tsx
import { Component } from 'valdi_core/src/Component';
import { HorizontalStack } from 'horizontal_stack/src/HorizontalStack';

export class GoodGreatBetterContainer extends Component {
  onRender() {
    <HorizontalStack>
      <view backgroundColor='lightyellow'>
        <label value='good:' />
        <slot name='good' />
      </view>
      <view backgroundColor='lightgreen'>
        <label value='great:' />
        <slot name='great' />
      </view>
      <view backgroundColor='green'>
        <label value='better:' />
        <slot name='better' />
      </view>
    </HorizontalStack>;
  }
}
```

And now, we can use that Component to slot in three different images.

> [!Note]
> The order in which we're declaring the `<slotted>` doesn't matter

```tsx
import { Component } from 'valdi_core/src/Component';
import { GoodGreatBetterContainer } from 'goodgreatbetter_container/src/GoodGreatBetterContainer';

export class HelloWorld extends Component {
  onRender() {
    <GoodGreatBetterContainer>
      <slotted slot='better'>
        <image width={88} height={88} margin={8} src='https://placecats.com/200/200' />
      </slotted>
      <slotted slot='good'>
        <image width={88} height={88} margin={8} src='https://placecats.com/210/210' />
      </slotted>
      <slotted slot='great'>
        <image width={88} height={88} margin={8} src='https://placecats.com/220/220' />
      </slotted>
    </GoodGreatBetterContainer>;
  }
}
```

![Screenshot of component rendering named slots](./assets/core-slots/IMG_1457.jpg)
