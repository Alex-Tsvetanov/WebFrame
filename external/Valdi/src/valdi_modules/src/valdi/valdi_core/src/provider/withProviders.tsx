import { Component } from '../Component';
import { ComponentConstructor, IComponent } from '../IComponent';
import { ProviderConstructor } from './ProviderComponent';
import { ProviderKey } from './ProviderKey';
import { resolveProviderValue } from './resolveProviderValue';

type ProviderValues<TProvidersValues> = {
  providersValues: TProvidersValues;
};

export type ProvidersValuesViewModel<TProvidersValues extends Array<unknown>> = ProviderValues<TProvidersValues>;
export type ProvidersConsumptionComponent<TOutViewModel> = IComponent<TOutViewModel>;

type GetProvidersValues<TProviderKeys> = {
  [Key in keyof TProviderKeys]: TProviderKeys[Key] extends ProviderKey<infer TProviderValue> ? TProviderValue : never;
};

type GetOutViewModel<TInViewModel> = Omit<TInViewModel, 'providersValues'>;

/**
 * Creates and returns a wrapper component/higher order component, which provides access to the provider's values.
 * To consume providers values you should pass needed providers to this function.
 * Then, you need to extend your view model from `ProvidersValuesViewModel` with the correct types of needed providers
 * Result of this function will be a wrapper/higher order component which will have the similar view model as your
 * original component but under the hood will pass an additional property as part of view model - providersValues.
 * This property represents actual providers values which will be resolved from the top/closest parent Provider components
 * How to create provider dependency, please see `createProvider` function.
 * For details and examples, please read the docs ../../../../../../../docs/docs/advanced-provider
 *
 * @param providerKeys The Provider Keys that were previously returned from a createProviderKey() call
 */
export function withProviderKeys<TProviderKeys extends readonly ProviderKey<any>[]>(
  ...providerSourceKeys: TProviderKeys
) {
  return function <
    TInViewModel extends ProviderValues<GetProvidersValues<TProviderKeys>>,
    TOutViewModel extends GetOutViewModel<TInViewModel>,
  >(
    ComponentForConsume: ComponentConstructor<IComponent<TInViewModel>, TInViewModel>,
  ): ComponentConstructor<IComponent<TOutViewModel>, TOutViewModel> {
    return class ProvidersConsumptionComponent extends Component<TOutViewModel> {
      providersValues = new Array<ProvidersValuesViewModel<unknown[]>>();

      onCreate(): void {
        this.providersValues = providerSourceKeys.map(resolveProviderValue);
      }

      onRender(): void {
        <ComponentForConsume
          {...(this.viewModel as TInViewModel & TOutViewModel)}
          providersValues={this.providersValues}
          children={(this.viewModel as any).children}
        />;
      }
    };
  };
}

/**
 * Creates and returns a wrapper component/higher order component, which provides access to the provider's values.
 * To consume providers values you should pass needed providers to this function.
 * Then, you need to extend your view model from `ProvidersValuesViewModel` with the correct types of needed providers
 * Result of this function will be a wrapper/higher order component which will have the similar view model as your
 * original component but under the hood will pass an additional property as part of view model - providersValues.
 * This property represents actual providers values which will be resolved from the top/closest parent Provider components
 * How to create provider dependency, please see `createProvider` function.
 * For details and examples, please read the docs ../../../../../../../docs/docs/advanced-provider
 *
 * @param providers The Provider Component classes that were previously returned from a createProvider() call
 */
export function withProviders<TProviders extends readonly ProviderConstructor<any>[]>(...providers: TProviders) {
  const providerKeys = providers.map(provider => provider.getProviderKey());
  return withProviderKeys(...providerKeys);
}
