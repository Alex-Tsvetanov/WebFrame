import { IComponentBase } from 'valdi_tsx/src/IComponentBase';
import { IRenderer } from './IRenderer';

export interface IComponent<ViewModel = any, Context = any> extends IComponentBase<ViewModel, Context> {
  readonly renderer: IRenderer;
}

export declare type ComponentConstructor<T extends IComponent<ViewModel, Context>, ViewModel = any, Context = any> = {
  new (renderer: IRenderer, viewModel: ViewModel, context: Context): T;

  disallowNullViewModel?: boolean;
};

// type ComponentConstructor<T extends IComponent> = Function & { prototype: T };
