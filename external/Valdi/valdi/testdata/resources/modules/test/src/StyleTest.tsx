import { Component } from 'valdi_core/src/Component';
import { Style } from 'valdi_core/src/Style';
import { View } from 'valdi_tsx/src/NativeTemplateElements';

const rootStyle = new Style<View>({backgroundColor: 'red', scaleX: 0.5, scaleY: 0.75});

export class StyleTest extends Component {

  onRender() {
    <view style={rootStyle}/>
  }

}