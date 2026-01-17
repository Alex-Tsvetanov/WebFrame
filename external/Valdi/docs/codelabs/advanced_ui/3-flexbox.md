# FlexBox Layout
If you've never done any web programming, `FlexBox` may be unfamiliar. Even if you have worked with the web before, you may need a refresher.

## FlexBox
`FlexBox` is a concept borrowed from CSS. It is an efficient way to layout children within a parent container even when their size is unknown or dynamic.

Valdi uses [Yoga](https://github.com/facebook/yoga), Facebook's cross-platform flexbox layout engine, to implement flexbox layout. The behavior closely follows the CSS Flexible Box Layout specification with some extensions.

> **For more details:** See [Core Flexbox](../../docs/core-flexbox.md) for an introduction and [Style Attributes Reference](../../api/api-style-attributes.md#flexbox-layout-attributes) for the complete API reference including Yoga-specific behaviors.

### Important terms
- **main axis**: in row mode this is horizontal, in column, it is vertical.
- **cross axis**: the axis perpendicular to the **main axis**. 

### FlexBox parent attributes
This is a selection of the most commonly used **FlexBox** attributes. You can find the complete list in the [Style Attributes API Reference](../../api/api-style-attributes.md#flexbox-layout-attributes).
- **`flexDirection`**: how the children should be arranged.
    - DEFAULT **`column`**: top to bottom
    - **`column-reverse`**: bottom to top
    - **`row`**: left to right
    - **`row-reverse`**: right to left
- **`justifyContent`**: alignment along the **main axis**.
    - DEFAULT **`flex-start`**: at the start
    - **`flex-end`**: at the end
    - **`center`**: at the center
    - **`space-between`**: evenly spaced
    - **`space-around`**: evenly space children. Children are centered with extra space on the left and right
    - **`space-evenly`**: evenly distributed. Space between each pair of items; the start edge and the first item; the end edge and the last item; are all the same.
- **`alignItems`**: alignment along the **cross axis**.
    - DEFAULT **`stretch`**: strtech children to to match height of the container
    - **`flex-start`**: at the start
    - **`flex-end`**: at the end
    - **`center`**: at the center
    - **`baseline`**: align children along a common baseline. An individual child can be set to be the reference baseline for the parent container.
- **`flexWrap`**: how to handle children that overflow the parent container.
    - DEFAULT **`no-wrap`**: no wrapping. Overflowing children will be hidden.
    - **`wrap`**: overflow the children into the next row or column based on **`flexDirection`**.
    - **`wrap-reverse`**: the same as **`wrap`** except the rows or columns are reversed.

### FlexBox child attributes
- **`flexGrow`**: how space should be distributed among the children along the main axis. After laying out the children, the parent will distribute any remaining space according to the **`flexGrow`** values specified by the children.
    - DEFAULT **`0`** 
    - float value `>= 0`: A greater value than other children, will give that child more of the extra space
- **`flexShrink`**: how the parent should constrain the children if they overflow the parent container. The opposite of **`flexGrow`**. **`flexGrow`** and **`flexShrink`** can work together within the same parent.
    - DEFAULT **`0`**
    - float value `>= 0`

## Display some dogs
Let's display a list of dogs so we can see how these attributes impact layout.

Let's render our dogs in a separate function to make it eaiser to update them.

```typescript
onRenderItems() {
    for (let i = 0; i < 3; i++) {
        const dog = 'https://placedog.net/50' + i;
        <image src={dog} height={64} width={64} border='1 solid red' />;
    }
}
```

The slightly different URLs will give us different dog images.

Now we can call this function from `onRender`.

```typescript
onRender() {
    <view padding='60 20' backgroundColor='green'>
        <view backgroundColor='lightblue' width={250} height={250}>
            {this.onRenderItems()}
        </view>
    </view>;
}
```

The outer `<view>` is the white background that you see. The inner `<view>` is the parent container that we're going to be playing with.

This layout is using all of the defaults. Checkout the UI, you should see a column of dogs.

## `flexDirection`
**`flexDirection`** defaults to column, let's see what this looks like as a row.

```typescript
<view backgroundColor='lightblue' width={250} height={250} flexDirection='row'>
    {this.onRenderItems()}
</view>
```

Now we have a row of dogs.

If we specify **`row-reverse`**, we can get them in the opposite order and lined up on the right.

```typescript
<view backgroundColor='lightblue' width={250} height={250} flexDirection='row-reverse'>
    {this.onRenderItems()}
</view>
```

## Combining multiple attributes
Going component by component is boring, let's do something more fun.

Duplicate the list of dogs so we have three of them and give the views a border so we can tell them apart.

```typescript
<view backgroundColor='lightblue' width={250} height={250} flexDirection='row-reverse' border='1 solid green'>
    {this.onRenderItems()}
</view>
<view backgroundColor='lightblue' width={250} height={250} flexDirection='row-reverse' border='1 solid green'>
    {this.onRenderItems()}
</view>
<view backgroundColor='lightblue' width={250} height={250} flexDirection='row-reverse' border='1 solid green'>
    {this.onRenderItems()}
</view>
```

Give 'em all different **`justifyContent`** values. 

```typescript
<view
    backgroundColor='lightblue'
    width={250}
    height={250}
    flexDirection='row-reverse'
    border='1 solid green'
    justifyContent='center'
>
    {this.onRenderItems()}
</view>
<view
    backgroundColor='lightblue'
    width={250}
    height={250}
    flexDirection='row-reverse'
    border='1 solid green'
    justifyContent='space-between'
>
    {this.onRenderItems()}
</view>
<view
    backgroundColor='lightblue'
    width={250}
    height={250}
    flexDirection='row-reverse'
    border='1 solid green'
    justifyContent='space-evenly'
>
    {this.onRenderItems()}
</view>
```

Check out the subtle difference between **`space-between`** and **`space-evenly`**.

Let's do the same with **`alignItems`** to place dogs on the cross axis.

```typescript
<view
    backgroundColor='lightblue'
    width={250}
    height={250}
    flexDirection='row-reverse'
    border='1 solid green'
    justifyContent='center'
    alignItems='flex-end'
>
    {this.onRenderItems()}
</view>
<view
    backgroundColor='lightblue'
    width={250}
    height={250}
    flexDirection='row-reverse'
    border='1 solid green'
    justifyContent='space-between'
    alignItems='center'
>
    {this.onRenderItems()}
</view>
<view
    backgroundColor='lightblue'
    width={250}
    height={250}
    flexDirection='row-reverse'
    border='1 solid green'
    justifyContent='space-evenly'
    alignItems='flex-start'
>
    {this.onRenderItems()}
</view>
```

Look at the UI, see that these values do something, go back and read the documentation above and make sure you understand why.

## Child spacing
Setting child attributes can create some interesting results.

Let's update our dog rendering so that we can specify an attribute on only one child.

```typescript
onRenderItems() {
    for (let i = 0; i < 3; i++) {
        const dog = 'https://placedog.net/50' + i;
        if (i == 1) {
            <image src={dog} height={64} width={64} border='1 solid blue' />;
        } else {
            <image src={dog} height={64} width={64} border='1 solid red' />;
        }
    }
}
```

Now we can do something different with the middle dog. Let's give him some grow.

```typescript
<image src={dog} height={64} width={64} border='1 solid blue' flexGrow={2} />;
```

Check out the UI, the middle dog is now taking up extra space relative to his siblings.

Now let's change that to shrink.

```typescript
<image src={dog} height={64} width={64} border='1 solid blue' flexShrink={2} />;
```

This looks the same as before. Why did nothing happen? **`flexShrink`** only has an impact when the children overflow the parent container and in our example, they all fit neatly in the parent.

Let's change that by making the dogs bigger.

```typescript
if (i == 1) {
    <image src={dog} height={100} width={100} border='1 solid blue' flexShrink={2} />;
} else {
    <image src={dog} height={100} width={100} border='1 solid red' />;
}
```

Now you should be able to see the impact of **`flexShrink`**. 

**`flexShrink`** and **`flexGrow`** can be used together in the same UI to create layouts that behave well in different sized parent containers.

## Final solution
Parking it here in case you need it.

```typescript
import { StatefulComponent } from 'valdi_core/src/Component';
import { ScrollViewHandler } from 'coreui/src/components/scroll/ScrollViewHandler';
import { SectionList } from 'coreui/src/components/section/SectionList';
import { RenderFunctionBody, RenderFunctionHeader, SectionModel } from 'coreui/src/components/section/SectionModel';
import { SemanticColor } from 'coreui/src/styles/semanticColors';

/**
 * Internal state of the component
 */
interface PlaygroundState {}

/**
 * @Component
 */
export class Playground extends StatefulComponent<{}, PlaygroundState> {
    onRender() {
        <view padding='60 20' backgroundColor={SemanticColor.Background.SUBSCREEN}>
        <view
            backgroundColor='lightblue'
            width={250}
            height={250}
            flexDirection='row-reverse'
            border='1 solid green'
            justifyContent='center'
            alignItems='flex-end'
        >
            {this.onRenderItems()}
        </view>
        <view
            backgroundColor='lightblue'
            width={250}
            height={250}
            flexDirection='row-reverse'
            border='1 solid green'
            justifyContent='space-between'
            alignItems='center'
        >
            {this.onRenderItems()}
        </view>
        <view
            backgroundColor='lightblue'
            width={250}
            height={250}
            flexDirection='row-reverse'
            border='1 solid green'
            justifyContent='space-evenly'
            alignItems='flex-start'
        >
            {this.onRenderItems()}
        </view>
        </view>;
    }
    onRenderItems() {
        for (let i = 0; i < 3; i++) {
            const dog = 'https://placedog.net/50' + i;
            if (i == 1) {
                <image src={dog} height={100} width={100} border='1 solid blue' flexShrink={2} />;
            } else {
                <image src={dog} height={100} width={100} border='1 solid red' />;
            }
        }
    }
}
```
