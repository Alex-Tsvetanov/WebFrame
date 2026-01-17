import { Component } from "valdi_core/src/Component";
import { RequireFunc } from "valdi_core/src/IModuleLoader";
import { makeInterruptibleCallbackOptionally } from 'valdi_core/src/utils/FunctionUtils';

declare const require: RequireFunc;

interface MakeInterruptibleCallbackParams {
  interruptible: boolean;
  cb: (cb: Function) => void;
}

export class AttributedError extends Component {

  shouldErrorOut = false;

  onRender() {
    if (this.shouldErrorOut) {
      throw new Error('Error in Render');
    }
    <view/>;
  }

  makeInterruptibleCallback(params: MakeInterruptibleCallbackParams) {
    params.cb(makeInterruptibleCallbackOptionally(params.interruptible, () => {
      return 42;
    }));
  }

  makeCallbackError(cb: (cb: Function) => void) {
    cb(() => {
        throw new Error('Error in Callback');
    });
  }

  makeRenderError() {
    this.shouldErrorOut = true;
    this.renderer.renderComponent(this, undefined);
  }

  makeImportError() {
    require('invalid_module/src/NotARealFile', true);
  }

  makeImportInitError() {
    require('test/src/ErrorModule', true);
  }

  makeSetTimeoutError() {
    setTimeout(() => {
        throw new Error('Error in setTimeout');
    });
  }

  makeDelayedImportError() {
    require('test/src/DelayedErrorModule', true);
  }

}
