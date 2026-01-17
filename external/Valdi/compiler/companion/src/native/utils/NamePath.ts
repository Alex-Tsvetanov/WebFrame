import ts = require('typescript');

export class NamePath {
  constructor(readonly name: string, readonly parent?: NamePath) {}

  appending(name: string): NamePath {
    return new NamePath(name, this);
  }

  appendingTSNode(node: ts.Node | undefined, prefix?: string | undefined): NamePath {
    if (!node || !ts.isIdentifier(node)) {
      return this.appending((prefix ?? '') + 'anon');
    }

    return this.appending((prefix ?? '') + node.text);
  }

  toString(): string {
    const components: string[] = [];
    let current: NamePath | undefined = this;
    while (current) {
      components.push(current.name);
      current = current.parent;
    }

    return components.reverse().join('_');
  }
}
