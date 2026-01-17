import { Component } from "valdi_core/src/Component";
import { ValdiRuntime } from "valdi_core/src/ValdiRuntime";

declare const runtime: ValdiRuntime;

export class ColorPaletteTest extends Component {

  onCreate() {
    runtime.setColorPalette({
      background: 'blue',
      foreground: 'green'
    });
  }

  onRender() {
    <view border='1 solid background'>
      <view background='foreground'/>
    </view>
  }

  updateColorPalette() {
    runtime.setColorPalette({
      background: 'red',
      foreground: 'yellow'
    });
  }
}