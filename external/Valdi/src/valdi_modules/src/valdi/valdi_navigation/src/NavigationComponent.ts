import { IComponent } from 'valdi_core/src/IComponent';
import { INavigator } from './INavigator';
import { NavigationController, NavigationOptions } from './NavigationController';

export interface NavigationPageContext {
  navigator: INavigator;
  pageNavigationOptions?: NavigationOptions;
}

export interface INavigationComponent extends IComponent {
  navigationController: NavigationController;
}

/**
 * Resolves the NavigationController from the current component.
 * Will throw if the navigator cannot be resolved
 * @param component
 */
export function getNavigationController(component: IComponent): NavigationController {
  const navigationComponent = getNavigationComponent(component);
  return navigationComponent.navigationController;
}

export function getNavigationComponent(component: IComponent): INavigationComponent {
  const navigationComponent = component as INavigationComponent;
  if (navigationComponent.navigationController) {
    return navigationComponent;
  }
  const parent = component.renderer.getComponentParent(component);
  if (!parent) {
    throw new Error('Could not resolve navigator');
  }
  return getNavigationComponent(parent);
}
