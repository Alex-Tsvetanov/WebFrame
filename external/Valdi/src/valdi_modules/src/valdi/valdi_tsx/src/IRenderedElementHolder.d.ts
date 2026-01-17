import { IRenderedElementBase } from './IRenderedElementBase';

export interface IRenderedElementHolder<T> {
  onElementCreated(element: IRenderedElementBase<T>): void;
  onElementDestroyed(element: IRenderedElementBase<T>): void;
  setElements(elements: IRenderedElementBase<T>[]): void;
}
