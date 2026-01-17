# Native annotations
Now that we have our calculator, we need to make it accessible from native code. In Valdi, this is done with annotations.

You can find a full list in the [Native Annotations documentation](../../docs/native-annotations.md).

## GenerateNativeInterface
We'll need the calculator interface.

```typescript
// @GenerateNativeInterface
export interface ICalculator {
  add(value: number): void;
  sub(value: number): void;
  mul(value: number): void;
  div(value: number): void;

  total(): number;
}
```

## GenerateNativeFunction
We also need the utility methods.

```typescript
// @GenerateNativeFunction
export function createCalculator(startingValue: number): ICalculator {
  return new SampleCalculator(startingValue);
}

// @GenerateNativeFunction
export function calculatorToString(calculator: ICalculator): string {
  const classType = calculator.constructor?.name ?? '<unknown class>';
  const isJsStringConvertible = calculator instanceof SampleCalculator;

  return `Class: ${classType} (is js: ${isJsStringConvertible}): ${calculator.total()}`;
}
```

These will create native files with the paths and prefixes specified in this module's `module.yaml`.

### Extra configuration options
There are more options available for the `@GenerateNativeFunction` annotation.

Let's play with them in `Magic.ts`.

```typescript
// @GenerateNativeFunction({ios: 'SCTotesIsMagic', android: 'com.valdi.hello.world.TotesIsMagic'})
export function showMeTheMagic(): string {
  return '42andThings';
}
```

Here we're specifying the iOS name and Android path for the function.


## Generate the native files
Because we've added `GenerateNative` annotations, we need to recompile and synchronize the project.

If you're only working on one module, you can specify it with the `--target` option to speed up the process. Keep in mind that the first compile you do after creating a new branch and linking modules needs to compile all of the modules. Only after you start iterating on one module can you speed up the process.

```
valdi projectsync
```

> **Note:** The `projectsync` command regenerates native bindings and updates VS Code project files. See [Command Line References](../../docs/command-line-references.md) for more details.

## The full solution if you need it
### Calculator.ts
```typescript
// @GenerateNativeInterface
export interface ICalculator {
  add(value: number): void;
  sub(value: number): void;
  mul(value: number): void;
  div(value: number): void;

  total(): number;
}

class SampleCalculator implements ICalculator {
  private current = 0;

  constructor(startingValue: number) {
    this.current = startingValue;
  }

  add(value: number): void {
    this.current += value;
  }

  sub(value: number): void {
    this.current -= value;
  }

  mul(value: number): void {
    this.current *= value;
  }

  div(value: number): void {
    this.current /= value;
  }

  total(): number {
    return this.current;
  }
}

// @GenerateNativeFunction
export function createCalculator(startingValue: number): ICalculator {
  return new SampleCalculator(startingValue);
}

// @GenerateNativeFunction
export function calculatorToString(calculator: ICalculator): string {
  const classType = calculator.constructor?.name ?? '<unknown class>';
  const isJsStringConvertible = calculator instanceof SampleCalculator;

  return `Class: ${classType} (is js: ${isJsStringConvertible}): ${calculator.total()}`;
}
```

### Magic.ts
```typescript
// @GenerateNativeFunction({ios: 'SCTotesIsMagic', android: 'com.valdi.hello.world.TotesIsMagic'})
export function showMeTheMagic(): string {
  return '42andThings';
}
```

## Choose your own adventure
This is the last step that applies to both Android and iOS. From here, pick one platform and follow the path to the end before coming back to do the other one (if you want).

### [iOS >](./ios/1-ios_setup_for_development.md)
### [Android >](./android/1-android_setup_for_development.md)

