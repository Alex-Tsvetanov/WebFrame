# Summary
This guide offers a comprehensive introduction to implementing control flow and loops within Valdi. It provides developers with the tools to manage conditional rendering and iterate through lists effectively in their UI designs.


# Control Flow and Loops in Valdi Components

## Component Setup
Begin with a basic Valdi component, initializing necessary variables in onCreate and handling logic in onRender

```tsx
import { Component } from 'valdi_core/src/Component';

export class GettingStartedCodelab extends Component {
    private msg?: string;

    onCreate() {
        this.msg = 'Hello Valdi on ';
    }

    onRender() {
        const fullMsg = this.msg + new Date().toLocaleTimeString();
        <layout>
            <label value='Declarative codelab' font="system-bold 24"  />
            <label value={fullMsg} font="system 16" />
        </layout>;
    }
}
```

## Conditional Rendering
### Using if Statements
Easily manage conditional rendering in your component with if statements:
```tsx
import { Component } from 'valdi_core/src/Component';

export class GettingStartedCodelab extends Component {
    private msg?: string;
    private kitten?: boolean;

    onRender() {
        if (this.kitten) {
            <image height={50} width={50} src='https://placecats.com/300/300' />
        } else {
            const fullMsg = this.msg + new Date().toLocaleTimeString();
            <layout>
                <label value='Declarative codelab' font="system-bold 24" />
                <label value={fullMsg} font="system 16" />
            </layout>;
        }
    }
}
```

### Nested Functions for Reusability
Improve readability and reusability by using nested functions for complex render logic:
```tsx
import { Component } from 'valdi_core/src/Component';

onRenderMessage(message: string): Component {
    if (this.kitten) {
        return <image height={50} width={50} src='https://placecats.com/300/300' />;
    }
    return <label value={message} font="system 16" />;
}

onRender() {
    const fullMsg = this.msg + new Date().toLocaleTimeString();
    <layout>
        <label value='Declarative codelab' font="system-bold 24" />
        {this.onRenderMessage(fullMsg)}
    </layout>;
}
```

## Inline **`if`** and **`for`** caveat
You might think, having seen the examples above, that you can do something similar with inline **`if`** and **`for`** loops.

Well, you can't.

The contents of a curly brace block inside of the TSX need to be an expression. In the current tsx standard, **`if`** and **`for`** are not expressions.
The alternatives are when and .forEach.


## **`when`** expressions
when is a replacement for if for inline rendering.

Going back to our **`if`** implementation.

```tsx
onRender() {
    if (this.kitten) {
        <image height={50} width={50} src='https://placecats.com/300/300' />
    } else {
        const fullMsg = this.msg + new Date().toLocaleTimeString();
        <layout>
            <label value='Declarative codelab' font="system-bold 24" />
            <label value={fullMsg} font="system 16" />
        </layout>;
    }
}
```

The first parameter in **`when`** is the conditional, this can be an expression that evaluates to a **`boolean`** or just a variable. The second parameter is a lambda that executes when your conditional evaluates to **`true`**.

**`when`** has no **`else`** clause so if you want that kind of thing, you need to implement a `when(!conditional)`.

```tsx
onRender() {
    const fullMsg = this.msg + new Date().toLocaleTimeString();
    <layout>
        <label value='Declarative codelab' font="system-bold 24" />
        {when(this.kitten, () => {
            <image height={50} width={50} src='https://placecats.com/300/300' />;
        })}
        {when(!this.kitten, () => {
            <label value={fullMsg} font="system 16" />;
        })}
    </layout>;
}
```

## Inline Loops
You can iterate through lists directly within the TSX using map :

```tsx
import { Component } from 'valdi_core/src/Component';

export class GettingStartedCodelab extends Component {
    private kittens = [
        'https://placecats.com/300/300',
        'https://placecats.com/300/301',
        'https://placecats.com/300/302',
    ];

    onRender() {
        <layout>
            <label value='Declarative codelab' font="system-bold 24" />
            {this.kittens.map(kitten => (
                <image height={50} width={50} src={kitten} />
            ))}
        </layout>;
    }
}
```

## Accessing a specific element in an array
Using a key to access a specific element in an array
```tsx
import { Component } from 'valdi_core/src/Component';

export class SampleApps extends Component<StartComponentViewModel, StartComponentContext> {
    private kittens = [
        { id: 0, url: 'https://picsum.photos/300/300?random=1' },
        { id: 1, url: 'https://picsum.photos/300/301?random=2' },
        { id: 2, url: 'https://picsum.photos/300/302?random=3' }
    ];

    onCreate(): void {
        console.log('Hello World onCreate!');
    }

    onRender() {
        <layout padding={10}>
            {this.kittens
                .filter(kitten => kitten.id === 2)
                .map(kitten => (
                    <image key={kitten.id.toString()} height={50} width={50} src={kitten.url} />
                ))}
        </layout>;
    }
}
```

