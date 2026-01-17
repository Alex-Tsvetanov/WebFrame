import { Component } from "valdi_core/src/Component";
import { RequireFunc } from 'valdi_core/src/IModuleLoader';

declare const require: RequireFunc;

// We inject this file only through hot reload.
const value = require('./NewFile', true).value as string;

export class MyComponent extends Component {

  onRender() {
    <view>
        <label value={value}/>
    </view>
  }
}
