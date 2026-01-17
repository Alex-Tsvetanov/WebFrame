import { IRenderedComponentHolder } from 'valdi_tsx/src/IRenderedComponentHolder';
import { ComponentConstructor, IComponent } from './IComponent';
import { IEntryPointComponent } from './IEntryPointComponent';
import { IRenderedVirtualNode } from './IRenderedVirtualNode';

export type EntryPointRenderFunction = (
  rootRef: IRenderedComponentHolder<IEntryPointComponent, IRenderedVirtualNode>,
  viewModel: any,
  context: any,
) => void;

export type EntryPointRenderFunctionFactory = (
  resolveConstructor: () => ComponentConstructor<IComponent>,
) => EntryPointRenderFunction;
