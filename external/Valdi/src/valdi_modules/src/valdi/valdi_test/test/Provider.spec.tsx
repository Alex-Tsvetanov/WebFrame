import 'jasmine/src/jasmine';
import { Component, StatefulComponent } from 'valdi_core/src/Component';
import { ComponentRef } from 'valdi_core/src/ComponentRef';
import {
  createGlobalProviderSource,
  getGlobalProviderSource,
  unregisterGlobalProviderSource,
} from 'valdi_core/src/provider/GlobalProviderSource';
import { ProviderComponent } from 'valdi_core/src/provider/ProviderComponent';
import { createProviderKey } from 'valdi_core/src/provider/ProviderKey';
import { WithGlobalProviderSource } from 'valdi_core/src/provider/WithGlobalProviderSource';
import { createProviderComponent, createProviderComponentWithKeyName } from 'valdi_core/src/provider/createProvider';
import {
  ProvidersConsumptionComponent,
  ProvidersValuesViewModel,
  withProviderKeys,
  withProviders,
} from 'valdi_core/src/provider/withProviders';

import { valdiIt, makeComponentTest } from './JSXTestUtils';

describe('Provider API', () => {
  interface TestProviderData {
    test: string;
  }

  function getConsumerComponent<TViewModel>(consumerRef: ComponentRef<ProvidersConsumptionComponent<TViewModel>>) {
    const consumerHocComponent = consumerRef.single();
    return consumerHocComponent?.renderer.getComponentChildren(consumerHocComponent)[0];
  }

  valdiIt('Provider render should return provider source data via getProviderSource', async driver => {
    const providerKey = createProviderKey<TestProviderData>('Test');
    const TestProvider = createProviderComponent(providerKey);
    const providerRef = new ComponentRef<ProviderComponent<TestProviderData>>();
    const testValue = { test: 'test' };

    driver.render(() => {
      <TestProvider ref={providerRef} value={testValue} />;
    });

    const providerSource = providerRef.single()?.$getProviderSource();
    expect(providerSource).toBeTruthy();

    expect(providerSource!.getValue(providerKey)).toEqual(testValue);
    expect(providerKey).toEqual(TestProvider.getProviderKey());
  });

  valdiIt('Consumer should get Provider value', async driver => {
    const TestProvider = createProviderComponentWithKeyName<TestProviderData>('Test');
    const testValue: TestProviderData = { test: 'test' };

    interface MyComponentViewModel extends ProvidersValuesViewModel<[TestProviderData]> {
      ownProperty: boolean;
    }

    class MyComponent extends Component<MyComponentViewModel> {}

    const MyComponentWithProviderData = withProviders(TestProvider)(MyComponent);

    const consumerRef = new ComponentRef<ProvidersConsumptionComponent<MyComponentViewModel>>();

    driver.render(() => {
      <TestProvider value={testValue}>
        <view>
          <MyComponentWithProviderData ref={consumerRef} ownProperty={true} />
        </view>
      </TestProvider>;
    });

    const consumerHocComponent = consumerRef.single();
    const consumerComponent = consumerHocComponent?.renderer.getComponentChildren(consumerHocComponent)[0];

    expect(consumerComponent?.viewModel.providersValues).toEqual([testValue]);
  });

  it('Consumer throw if there is no Provider on the top', async () => {
    const TestProvider = createProviderComponentWithKeyName<TestProviderData>('Test');

    interface MyComponentViewModel extends ProvidersValuesViewModel<[TestProviderData]> {
      ownProperty: boolean;
    }

    class MyComponent extends Component<MyComponentViewModel> {}

    const MyComponentWithProviderData = withProviders(TestProvider)(MyComponent);

    const test = makeComponentTest(async driver => {
      driver.render(() => <MyComponentWithProviderData ownProperty={true} />);
    });

    await expectAsync(test()).toBeRejectedWithError(
      'No ProviderSource found in the tree. Please render a Provider component within this component tree',
    );
  });

  valdiIt('Update Provider value should trigger the update for render tree', async driver => {
    const TestProvider = createProviderComponentWithKeyName<TestProviderData>('Test');
    const consumerRef = new ComponentRef<ProvidersConsumptionComponent<MyComponentViewModel>>();
    const providerRef = new ComponentRef<MyRootComponent>();

    interface MyComponentViewModel extends ProvidersValuesViewModel<[TestProviderData]> {
      ownProperty: boolean;
    }

    class MyComponent extends Component<MyComponentViewModel> {}

    const MyComponentWithProviderData = withProviders(TestProvider)(MyComponent);

    class MyRootComponent extends StatefulComponent<unknown, TestProviderData> {
      state: TestProviderData = { test: 'init' };

      updateProviderValue() {
        this.setState({ test: 'update' });
      }

      onRender() {
        <TestProvider value={this.state}>
          <view>
            <MyComponentWithProviderData ref={consumerRef} ownProperty={true} />
          </view>
        </TestProvider>;
      }
    }

    driver.render(() => <MyRootComponent ref={providerRef} />);

    const componentInInitialState = getConsumerComponent(consumerRef);

    expect(componentInInitialState?.viewModel.providersValues).toEqual([{ test: 'init' }]);

    providerRef.single()?.updateProviderValue();

    const componentInFinalState = getConsumerComponent(consumerRef);

    expect(componentInFinalState?.viewModel.providersValues).toEqual([{ test: 'update' }]);
  });

  it('can register and retrieve GlobalProvider', () => {
    const providerKey = createProviderKey<TestProviderData>('Test');
    const testValue: TestProviderData = { test: 'test' };
    const globalProviderSourceId = createGlobalProviderSource(undefined, providerSource =>
      providerSource.withValue(providerKey, testValue),
    );
    try {
      const providerSource = getGlobalProviderSource(globalProviderSourceId);

      expect(providerSource).toBeTruthy();

      expect(providerSource?.getValue(providerKey)).toBe(testValue);

      unregisterGlobalProviderSource(globalProviderSourceId);

      expect(getGlobalProviderSource(globalProviderSourceId)).toBeUndefined();
    } finally {
      unregisterGlobalProviderSource(globalProviderSourceId);
    }
  });

  it('can register multiple GlobalProvider values', () => {
    const providerKey1 = createProviderKey<TestProviderData>('Test');
    const providerKey2 = createProviderKey<string>('websiteURL');
    const providerKey3 = createProviderKey<number>('maxTransactionsCount');
    const providerKey4 = createProviderKey<string>('Unused');

    const testValue: TestProviderData = { test: 'test' };

    const globalProviderSourceId1 = createGlobalProviderSource(undefined, providerSource =>
      providerSource.withValue(providerKey1, testValue),
    );
    const globalProviderSourceId2 = createGlobalProviderSource(globalProviderSourceId1, providerSource =>
      providerSource.withValue(providerKey2, 'https://snapchat.com').withValue(providerKey3, 42),
    );
    try {
      const globalProvider1 = getGlobalProviderSource(globalProviderSourceId1)!;
      expect(globalProvider1).toBeTruthy();

      // First global provider should only have testValue set
      expect(globalProvider1.getValue(providerKey1)).toBe(testValue);
      expect(() => {
        globalProvider1.getValue(providerKey2);
      }).toThrow();
      expect(() => {
        globalProvider1.getValue(providerKey3);
      }).toThrow();
      expect(() => {
        globalProvider1.getValue(providerKey4);
      }).toThrow();

      const globalProvider2 = getGlobalProviderSource(globalProviderSourceId2)!;
      expect(globalProvider2).toBeTruthy();

      // Second global provider should have everything beside providerKey4
      expect(globalProvider2.getValue(providerKey1)).toBe(testValue);
      expect(globalProvider2.getValue(providerKey2)).toBe('https://snapchat.com');
      expect(globalProvider2.getValue(providerKey3)).toBe(42);
      expect(() => {
        globalProvider1.getValue(providerKey4);
      }).toThrow();
    } finally {
      unregisterGlobalProviderSource(globalProviderSourceId1);
      unregisterGlobalProviderSource(globalProviderSourceId2);
    }
  });

  valdiIt('Can resolve values through a global ProviderSource', async driver => {
    const providerKey = createProviderKey<TestProviderData>('Test');
    const testValue: TestProviderData = { test: 'test' };
    const globalProviderSourceId = createGlobalProviderSource(undefined, providerSource =>
      providerSource.withValue(providerKey, testValue),
    );

    try {
      interface MyComponentViewModel extends ProvidersValuesViewModel<[TestProviderData]> {
        ownProperty: boolean;
      }

      class MyComponent extends Component<MyComponentViewModel> {}

      const MyComponentWithProviderData = withProviderKeys(providerKey)(MyComponent);

      const consumerRef = new ComponentRef<ProvidersConsumptionComponent<MyComponentViewModel>>();

      driver.render(() => {
        <WithGlobalProviderSource globalProviderSourceId={globalProviderSourceId}>
          <view>
            <MyComponentWithProviderData ref={consumerRef} ownProperty={true} />
          </view>
        </WithGlobalProviderSource>;
      });

      const consumerHocComponent = consumerRef.single();
      const consumerComponent = consumerHocComponent?.renderer.getComponentChildren(consumerHocComponent)[0];

      expect(consumerComponent?.viewModel.providersValues).toEqual([testValue]);

      driver.render(() => {});
    } finally {
      unregisterGlobalProviderSource(globalProviderSourceId);
    }
  });
});
