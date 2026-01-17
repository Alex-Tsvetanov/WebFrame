import { Component } from 'valdi_core/src/Component';

interface ViewModel {
  src: string;
}

export class RemoteAsset extends Component<ViewModel> {

  onRender() {
    <view>
      <image id='imageView' src={this.viewModel.src}/>
    </view>
  }

}