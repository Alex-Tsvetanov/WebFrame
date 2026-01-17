# Business logic
Let's implement our business logic for this new module we've created.

## Calculator 
We're going to create a basic calculator in typescript.

In `Calculator.ts`, create the basic interface.

```typescript
export interface ICalculator {
  add(value: number): void;
  sub(value: number): void;
  mul(value: number): void;
  div(value: number): void;

  total(): number;
}
```

The `export` makes this interface available outside of the file.

Then, in the same file, we'll create a simple implementation.

```typescript
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
```

Then we need some utility functions so we can use this implementation.

```typescript
export function createCalculator(startingValue: number): ICalculator {
  return new SampleCalculator(startingValue);
}

export function calculatorToString(calculator: ICalculator): string {
  const classType = calculator.constructor?.name ?? '<unknown class>';
  const isJsStringConvertible = calculator instanceof SampleCalculator;

  return `Class: ${classType} (is js: ${isJsStringConvertible}): ${calculator.total()}`;
}
```

## Magic
A persistent calculator is all fine and good but what if you need a one-off cross-platform function to access some magic numbers?

Create another typescript file, `Magic.ts`.

In this file, we'll implement another function that returns a magic string.

```typescript
export function showMeTheMagic(): string {
  return '42andThings';
}
```

### [Next >](./4-native_annotations.md)
