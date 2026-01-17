import res from 'test/res';
import { Component } from 'valdi_core/src/Component';

export class BundledAsset extends Component<{}> {

  onRender() {
    <view>
      <image id='imageView' src={res.emoji}/>
    </view>
  }

}