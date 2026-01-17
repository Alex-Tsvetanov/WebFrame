import { IComponent } from '../IComponent';
import { jsx } from '../JSXBootstrap';
import { IProviderSource } from './IProviderSource';
import { ProviderComponent } from './ProviderComponent';

function findProviderSource(currentComponent: IComponent | undefined): IProviderSource | undefined {
  while (currentComponent) {
    const providerComponent = currentComponent as ProviderComponent<any>;
    if (providerComponent.$getProviderSource) {
      const providerSource = providerComponent.$getProviderSource();
      if (providerSource) {
        return providerSource;
      }
    }

    currentComponent = currentComponent.renderer.getComponentParent(currentComponent);
  }

  return undefined;
}

export function resolveProviderSource(): IProviderSource | undefined {
  const currentComponent = jsx.getCurrentComponentInstance();
  if (!currentComponent) {
    throw new Error('Cannot resolve Provider Source: there are no components being currently rendered');
  }

  return findProviderSource(jsx.getCurrentComponentInstance());
}
