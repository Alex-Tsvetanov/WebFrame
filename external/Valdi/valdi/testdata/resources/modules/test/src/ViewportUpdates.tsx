import { ElementFrame } from 'valdi_tsx/src/Geometry';
import { Component } from 'valdi_core/src/Component';

interface ViewModel {
  onViewportChanged: (frame: ElementFrame) => void;
}

export class ViewportUpdates extends Component<ViewModel> {

  onRender() {
    <view>
      <scroll width='100%' height='100%'>
        <view width={100} height={100} onViewportChanged={this.viewModel.onViewportChanged}/>
        <layout width={100} height={100}/>
      </scroll>
    </view>
  }
}
