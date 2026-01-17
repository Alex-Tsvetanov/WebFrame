import { ElementRef } from 'valdi_core/src/ElementRef';
import { NativeView } from 'valdi_tsx/src/NativeView';
/**
 * Make a factory for callbacks that will bind the native view of the element ref as the callback argument
 */
export function makeFactoryForEventWithNativeView(elementRefGetter: () => ElementRef) {
  let actualCallback: ((v: NativeView) => void) | undefined = undefined;
  const wrappedCallback = async () => {
    if (!actualCallback) {
      return;
    }
    const elementRef = elementRefGetter();
    const nativeView = await (elementRef.single()?.getNativeView() ?? Promise.resolve(undefined));
    if (nativeView) {
      actualCallback(nativeView);
    }
  };
  return (callback?: (v: NativeView) => void) => {
    actualCallback = callback;
    if (actualCallback) {
      return wrappedCallback;
    }
    return undefined;
  };
}
