import { ComponentCtor, IRendererEventListener } from '../IRendererEventListener';

const enum RendererTraceEntryType {
  renderBegin = 1,
  renderEnd,
  componentBegin,
  componentEnd,
  bypassComponentRender,
  componentViewModelPropertyChange,
}

export class RendererEventsToString implements IRendererEventListener {
  // private buffer = '';
  private depth = 0;
  private minDepth = 0;
  private entries: (number | string)[] = [];
  private componentNamesStack: string[] = [];

  toString(): string {
    const depthOffset = Math.abs(this.minDepth);
    const entries = this.entries;
    const length = entries.length;
    let buffer = '';
    let i = 0;
    while (i < length) {
      const depth = (entries[i++] as number) + depthOffset;
      const content = entries[i++] as string;

      for (let i = 0; i < depth; i++) {
        buffer += '  ';
      }

      buffer += content;

      buffer += '\n';
    }

    return buffer;
  }

  private appendLine(content: string) {
    this.entries.push(this.depth, content);
  }

  onRenderBegin(): void {
    this.appendLine('Begin render');
    this.depth++;
  }

  onRenderEnd(): void {
    this.decrementDepth();
    this.appendLine('End render');
  }

  onComponentBegin(key: string, componentCtor: ComponentCtor): void {
    const componentName = componentCtor.name;
    this.appendLine(`Begin ${componentName} (key: ${key})`);
    this.componentNamesStack.push(componentName);
    this.depth++;
  }

  onComponentEnd(): void {
    const componentName = this.componentNamesStack.pop();
    this.decrementDepth();
    this.appendLine(`End ${componentName ?? '<unknown>'}`);
  }

  private decrementDepth(): void {
    this.minDepth = Math.min(this.minDepth, --this.depth);
  }

  onBypassComponentRender(): void {
    this.appendLine('Bypass render');
  }

  onComponentViewModelPropertyChange(viewModelPropertyName: string): void {
    this.appendLine(`ViewModel property '${viewModelPropertyName}' changed`);
  }
}

export class RendererEventRecorder implements IRendererEventListener {
  private buffer: unknown[] = [];
  private maxBufferSize = 0;

  clear() {
    this.buffer = [];
  }

  /**
   * Set a maximum underlying buffer size. The buffer will be automatically
   * shrinked after render ends so that it fits within the given bounds.
   */
  setMaxBufferSize(maxBufferSize: number): void {
    if (maxBufferSize !== this.maxBufferSize) {
      this.maxBufferSize = maxBufferSize;
      this.shrinkToMaxBufferSizeIfNeeded();
    }
  }

  toString(): string {
    const toStringVisitor = new RendererEventsToString();
    this.visit(toStringVisitor);
    return toStringVisitor.toString();
  }

  visit(visitor: IRendererEventListener): void {
    const buffer = this.buffer;
    const length = buffer.length;
    let i = 0;
    while (i < length) {
      const type = buffer[i++] as RendererTraceEntryType;
      switch (type) {
        case RendererTraceEntryType.renderBegin:
          visitor.onRenderBegin();
          break;
        case RendererTraceEntryType.renderEnd:
          visitor.onRenderEnd();
          break;
        case RendererTraceEntryType.componentBegin:
          {
            const key = buffer[i++] as string;
            const componentCtor = buffer[i++] as ComponentCtor;
            visitor.onComponentBegin(key, componentCtor);
          }
          break;
        case RendererTraceEntryType.componentEnd:
          visitor.onComponentEnd();
          break;
        case RendererTraceEntryType.bypassComponentRender:
          visitor.onBypassComponentRender();
          break;
        case RendererTraceEntryType.componentViewModelPropertyChange:
          {
            const viewModelPropertyName = buffer[i++] as string;
            visitor.onComponentViewModelPropertyChange(viewModelPropertyName);
          }
          break;
        default:
          throw new Error(`Invalid type: ${type}`);
      }
    }
  }

  private shrinkToMaxBufferSizeIfNeeded(): void {
    const buffer = this.buffer;
    const length = buffer.length;
    const maxBufferSize = this.maxBufferSize;
    if (maxBufferSize) {
      let i = 0;
      while (length - i > maxBufferSize) {
        const type = buffer[i++] as RendererTraceEntryType;
        switch (type) {
          case RendererTraceEntryType.renderBegin:
            break;
          case RendererTraceEntryType.renderEnd:
            break;
          case RendererTraceEntryType.componentBegin:
            i += 2;
            break;
          case RendererTraceEntryType.componentEnd:
            break;
          case RendererTraceEntryType.bypassComponentRender:
            break;
          case RendererTraceEntryType.componentViewModelPropertyChange:
            i++;
            break;
          default:
            throw new Error(`Invalid type: ${type}`);
        }
      }

      if (i > 0) {
        buffer.splice(0, i);
      }
    }
  }

  onRenderBegin(): void {
    this.buffer.push(RendererTraceEntryType.renderBegin);
  }

  onRenderEnd(): void {
    this.buffer.push(RendererTraceEntryType.renderEnd);
    this.shrinkToMaxBufferSizeIfNeeded();
  }

  onComponentBegin(key: string, componentCtor: ComponentCtor): void {
    this.buffer.push(RendererTraceEntryType.componentBegin, key, componentCtor);
  }

  onComponentEnd(): void {
    this.buffer.push(RendererTraceEntryType.componentEnd);
  }

  onBypassComponentRender(): void {
    this.buffer.push(RendererTraceEntryType.bypassComponentRender);
  }

  onComponentViewModelPropertyChange(viewModelPropertyName: string): void {
    this.buffer.push(RendererTraceEntryType.componentViewModelPropertyChange, viewModelPropertyName);
  }
}
