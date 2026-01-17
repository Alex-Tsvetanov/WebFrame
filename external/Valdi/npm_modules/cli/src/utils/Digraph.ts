export interface DigraphNode {
  name: string;
  edges: DigraphNode[];
}

function unquote(str: string): string {
  return str.slice(1, -1);
}

export class Digraph {
  private nodeByName = new Map<string, DigraphNode>();
  private _allNodes: DigraphNode[] = [];

  constructor() {}

  get nodes(): readonly DigraphNode[] {
    return this._allNodes;
  }

  static parse(str: string): Digraph {
    const lines = str.split('\n');
    const graph = new Digraph();

    let index = 0;
    for (const line of lines) {
      const trimmedLine = line.trim();
      if (trimmedLine.startsWith('digraph ')) {
        if (index !== 0) {
          this.throwInvalidGraph();
        }
      } else if (trimmedLine === '}') {
        if (index !== lines.length - 1) {
          this.throwInvalidGraph();
        }
      } else if (trimmedLine.startsWith('node ')) {
          // no-op
      } else if (trimmedLine.startsWith('"')) {
          const edgeIndex = trimmedLine.indexOf('->');
          if (edgeIndex >= 0) {
            const fromEdge = unquote(trimmedLine.slice(0, edgeIndex).trim());
            const toEdge = unquote(trimmedLine.slice(edgeIndex + 2).trim());
            graph.addEdge(fromEdge, toEdge);
          } else {
            graph.getOrCreateNode(unquote(trimmedLine))
          }
      } else {
        this.throwInvalidGraph();
      }

      index++;
    }

    return graph;
  }

  private static throwInvalidGraph(): never {
    throw new Error('Could not parse digraph');
  }

  getNode(name: string): DigraphNode | undefined {
    return this.nodeByName.get(name);
  }

  getOrCreateNode(name: string): DigraphNode {
    let node = this.nodeByName.get(name);
    if (!node) {
      node = { name, edges: [] };
      this.nodeByName.set(name, node);
      this._allNodes.push(node);
    }

    return node;
  }

  addEdge(from: string, to: string): void {
    const fromNode = this.getOrCreateNode(from);
    const toNode = this.getOrCreateNode(to);

    fromNode.edges.push(toNode);
  }
}
