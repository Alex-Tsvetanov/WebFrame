export class UnreachableCaseError extends Error {
  constructor(val?: any) {
    const message = `Unreachable case` + val ? `:${val}` : '';
    super(message);
  }
}

export function unreachable(val: any): any {
  throw new UnreachableCaseError(val);
}
