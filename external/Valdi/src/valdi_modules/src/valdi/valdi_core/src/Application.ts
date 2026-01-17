import {
  ApplicationCancelable,
  observeKeyboardHeight as nativeObserveKeyboardHeight,
  observeEnteredBackground as nativeObserveEnteredBackground,
  observeEnteredForeground as nativeObserveEnteredForeground,
  isForegrounded as nativeIsForegrounded,
  isIntegrationTestEnvironment as nativeIsIntegrationTestEnvironment,
  getAppVersion as nativeGetAppVersion,
} from './ApplicationBridge';
import { getModuleLoader } from './ModuleLoaderGlobal';

let currentKeyboardHeight = 0;
if (nativeObserveKeyboardHeight) {
  const observeKeyboardHeight = nativeObserveKeyboardHeight((height: number) => {
    currentKeyboardHeight = height;
  });
  getModuleLoader().onHotReload(module, module.path, () => {
    observeKeyboardHeight.cancel();
  });
}

/**
 * Public API for Application calls
 */
export namespace Application {
  /**
   * Start listening to the application entering the background
   */
  export function observeEnteredBackground(observe: () => void): ApplicationCancelable {
    if (nativeObserveEnteredBackground) {
      return nativeObserveEnteredBackground(observe);
    } else {
      console.error('Application.observeEnteredBackground not implemented');
      return {
        cancel: () => {},
      };
    }
  }
  /**
   * Start listening to the application entering the foreground
   */
  export function observeEnteredForeground(observe: () => void): ApplicationCancelable {
    if (nativeObserveEnteredForeground) {
      return nativeObserveEnteredForeground(observe);
    } else {
      console.error('Application.observeEnteredForeground not implemented');
      return {
        cancel: () => {},
      };
    }
  }
  /**
   * Start listening to the application's soft keyboard's height
   */
  export function observeKeyboardHeight(observe: (height: number) => void): ApplicationCancelable {
    if (nativeObserveKeyboardHeight) {
      return nativeObserveKeyboardHeight(observe);
    } else {
      console.error('Application.observeKeyboardHeight not implemented');
      observe(0);
      return {
        cancel: () => {},
      };
    }
  }
  /**
   * Get whether the application is currently foregrounded
   */
  export function isForegrounded(): boolean {
    if (nativeIsForegrounded) {
      return nativeIsForegrounded();
    } else {
      console.error('Application.isForegrounded not implemented');
      return true;
    }
  }

  /**
   * Get whether the application is running in a test environment
   * On Android this checks isCremaTestAutomationMode()
   * On iOS this checks if XCTestCase is available
   */
  export function isIntegrationTestEnvironment(): boolean {
    if (nativeIsIntegrationTestEnvironment) {
      return nativeIsIntegrationTestEnvironment();
    } else {
      console.error('Application.isTestEnvironment not implemented');
      return false;
    }
  }

  /**
   * Get the application version
   * e.g.: 1.0.3
   */
  export function getAppVersion(): string {
    if (nativeGetAppVersion) {
      return nativeGetAppVersion();
    } else {
      console.error('Application.appVersion not implemented');
      return '';
    }
  }

  /**
   * Get the current keyboard height
   */
  export function getKeyboardHeight(): number {
    return currentKeyboardHeight;
  }
}
