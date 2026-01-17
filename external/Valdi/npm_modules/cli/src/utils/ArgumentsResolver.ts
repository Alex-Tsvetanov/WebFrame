export class ArgumentsResolver<Args> {
  additionalResolvedArguments: Partial<Args> = {};

  constructor(readonly args: Args) {}

  getArgument<K extends keyof Args>(key: K): Args[K] {
    return this.args[key];
  }

  async getArgumentOrResolve<K extends keyof Args>(
    key: K,
    resolve: () => Promise<NonNullable<Args[K]>>,
  ): Promise<NonNullable<Args[K]>> {
    const value = this.args[key];
    if (value) {
      return value;
    }

    const resolved = await resolve();
    this.additionalResolvedArguments[key] = resolved;
    return resolved;
  }
}
