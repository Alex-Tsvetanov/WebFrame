import { IRenderedComponentHolder } from 'valdi_tsx/src/IRenderedComponentHolder';
import { Component } from './Component';
import { ComponentPrototype } from './ComponentPrototype';
import { EntryPointRenderFunction } from './EntryPointRenderFunction';
import { ComponentConstructor, IComponent } from './IComponent';
import { IEntryPointComponent } from './IEntryPointComponent';
import { IRenderedVirtualNode } from './IRenderedVirtualNode';
import { IRootElementObserver } from './IRootElementObserver';
import { jsx } from './JSXBootstrap';
import { DaemonClientManager } from './debugging/DaemonClientManager';
import { DebugConsole } from './debugging/DebugConsole';
import { DefaultErrorBoundary } from './debugging/DefaultErrorBoundary';

interface EntryPointViewModel {
  rootConstructorResolver: () => ComponentConstructor<IComponent>;
  rootViewModel: any;
  rootContext: any;
  daemonClientManager?: DaemonClientManager;
  enableErrorBoundary: boolean;
}

export class EntryPointComponent
  extends Component<EntryPointViewModel>
  implements IEntryPointComponent, IRenderedComponentHolder<IComponent, IRenderedVirtualNode>, IRootElementObserver
{
  private rootConstructor: ComponentConstructor<IComponent> | undefined = undefined;

  private rootPrototype: ComponentPrototype = jsx.makeComponentPrototype();

  rootComponent: IComponent | undefined = undefined;

  onCreate() {
    this.renderer.addObserver(this);
  }

  onDestroy() {
    this.renderer.removeObserver(this);
  }

  onRender() {
    if (this.viewModel.enableErrorBoundary) {
      <DefaultErrorBoundary>{this.renderRootComponent()}</DefaultErrorBoundary>;
    } else {
      this.renderRootComponent();
    }
  }

  forwardCall(name: string, parameters: any[]): any {
    const rootComponent = this.rootComponent;
    if (!rootComponent) {
      throw new Error(`Could not resolve component to call ${name}`);
    }

    const func = (rootComponent as any)[name] as (...parameters: any[]) => any;
    if (!func) {
      throw new Error(`Could not resolve function ${name}`);
    }

    return func.apply(rootComponent, parameters);
  }

  onComponentDidCreate(component: IComponent) {
    this.rootComponent = component;
  }

  onComponentWillDestroy(component: IComponent) {
    this.rootComponent = undefined;
  }

  private renderRootComponent() {
    const viewModel = this.viewModel;

    let RootConstructor = this.rootConstructor;
    if (!RootConstructor) {
      RootConstructor = viewModel.rootConstructorResolver();
      this.rootConstructor = RootConstructor;
    }

    ///
    /// <DANGER>
    ///
    /// Manually written "post-processed" TSX calls to use the setViewModelFull
    /// api we don't currently have a way to access with the TSX syntax.
    jsx.beginComponent(RootConstructor, this.rootPrototype, undefined, this);
    if (!jsx.hasContext()) {
      jsx.setContext(viewModel.rootContext);
    }
    jsx.setViewModelFull(viewModel.rootViewModel);
    jsx.endComponent();
    ///
    /// </DANGER>
    ///
  }

  onRootElementWillEndRender() {
    if (this.viewModel.daemonClientManager) {
      <DebugConsole daemonClientManager={this.viewModel.daemonClientManager} />;
    }
  }
}

export function makeEntryPointRenderFunction(
  resolveConstructor: () => ComponentConstructor<IComponent>,
  enableErrorBoundary: boolean,
  daemonClientManager: DaemonClientManager | undefined,
): EntryPointRenderFunction {
  return (rootRef, viewModel, context) => {
    <EntryPointComponent
      ref={rootRef}
      rootConstructorResolver={resolveConstructor}
      enableErrorBoundary={enableErrorBoundary}
      daemonClientManager={daemonClientManager}
      rootViewModel={viewModel}
      rootContext={context}
    />;
  };
}
