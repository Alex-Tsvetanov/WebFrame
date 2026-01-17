import { ComponentConstructor, IComponent } from '../IComponent';
import { IProviderSource } from './IProviderSource';
import { ProviderKey } from './ProviderKey';

export interface ProviderSourceComponent<ViewModel> extends IComponent<ViewModel> {
  $getProviderSource(): IProviderSource | undefined;
}

export type ProviderValue<TValue> = Readonly<TValue>;

export interface ProviderViewModel<TValue> {
  value: ProviderValue<TValue>;
}

export interface ProviderComponent<TValue> extends ProviderSourceComponent<ProviderViewModel<TValue>> {}

export interface ProviderConstructor<TValue>
  extends ComponentConstructor<ProviderComponent<TValue>, ProviderViewModel<TValue>> {
  getProviderKey(): ProviderKey<TValue>;
}
