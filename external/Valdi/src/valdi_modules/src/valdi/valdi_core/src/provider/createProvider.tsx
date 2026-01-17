import { StatefulComponent } from '../Component';
import { IProviderSource } from './IProviderSource';
import { ProviderComponent, ProviderConstructor, ProviderViewModel } from './ProviderComponent';
import { createProviderKey, ProviderKey, ProviderKeyName } from './ProviderKey';
import { ProviderSourceHead } from './ProviderSource';
import { resolveProviderSource } from './resolveProviderSource';

interface ProviderComponentState {
  providerValueKey: number;
  providerSource?: IProviderSource;
}

/**
 * "Creates and returns a Provider Component class that can be rendered to implicitly provide dependency to children components.
 * The returned class should be stored globally at the top of a TypeScript module.
 * To provide a dependency, you can render a Provider component with its value like <TestProvider value={testValue}>.
 * Whenever you re-render a Provider component with a new value, it will trigger a full re-render for all children components,
 * tearing down any children that were rendered with the previously provided value.
 * For more information about how to consume a provider dependency, please see the withProviders function.
 * For details and examples, please read the docs at ../../../../../../../docs/docs/advanced-provider"
 *
 * @param providerKeyName A Unique name key for the provider which uniquely identifies the provider
 */
export function createProviderComponentWithKeyName<TValue>(
  providerKeyName: ProviderKeyName,
): ProviderConstructor<TValue> {
  const key = createProviderKey<TValue>(providerKeyName);
  return createProviderComponent(key);
}

/**
 * "Creates and returns a Provider Component class that can be rendered to implicitly provide dependency to children components.
 * The returned class should be stored globally at the top of a TypeScript module.
 * To provide a dependency, you can render a Provider component with its value like <TestProvider value={testValue}>.
 * Whenever you re-render a Provider component with a new value, it will trigger a full re-render for all children components,
 * tearing down any children that were rendered with the previously provided value.
 * For more information about how to consume a provider dependency, please see the withProviders function.
 * For details and examples, please read the docs at ../../../../../../../docs/docs/advanced-provider"
 *
 * @param providerKey The ProviderKey which was returned from a previous call of createProviderKey()
 */
export function createProviderComponent<TValue>(providerKey: ProviderKey<TValue>): ProviderConstructor<TValue> {
  class Provider
    extends StatefulComponent<ProviderViewModel<TValue>, ProviderComponentState>
    implements ProviderComponent<TValue>
  {
    public state: ProviderComponentState = { providerValueKey: 0 };

    private startingProviderSource: IProviderSource | undefined;

    public onCreate(): void {
      super.onCreate();

      let providerSource = resolveProviderSource();
      if (!providerSource) {
        providerSource = new ProviderSourceHead();
      }
      this.startingProviderSource = providerSource;

      this.state = {
        providerValueKey: 0,
        providerSource: providerSource,
      };
    }

    public onViewModelUpdate(): void {
      const providerSource = this.startingProviderSource!;

      let { providerValueKey } = this.state;
      const subProviderSource = providerSource.withValue(providerKey, this.viewModel.value);

      this.setState({ providerValueKey: ++providerValueKey, providerSource: subProviderSource });
    }

    public $getProviderSource(): IProviderSource | undefined {
      return this.state.providerSource;
    }

    onRender(): void {
      <slot key={this.state.providerValueKey.toString()} />;
    }

    static getProviderKey(): ProviderKey<TValue> {
      return providerKey;
    }
  }

  Object.defineProperty(Provider, 'name', { value: `${providerKey.keyName}Provider` });

  return Provider;
}
