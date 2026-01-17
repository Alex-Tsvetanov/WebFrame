import { PropertyList } from './PropertyList';

export type NativeStyle = number;

export type StyleToNativeFunc = (attributes: PropertyList) => NativeStyle;

export interface IStyle<T> {
  readonly attributes: Omit<T, 'style'>;

  toNative(convertFunc: StyleToNativeFunc): NativeStyle;
}
