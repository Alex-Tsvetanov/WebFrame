/**
 * This class will act like a function call whose output is automatically cached
 * until the input value changes
 *
 * You can use this class as a simple caching object for expensive function calls and computations
 */
export class CachedFunctionCall<Input, Output> {
  private lastInput?: Input;
  private lastOutput?: Output;

  private fn: (input: Input) => Output;

  constructor(fn: (input: Input) => Output) {
    this.fn = fn;
  }

  getOrCall(input: Input): Output | undefined {
    if (input !== this.lastInput) {
      this.lastInput = input;
      this.lastOutput = this.fn(input);
    }
    return this.lastOutput;
  }
}
