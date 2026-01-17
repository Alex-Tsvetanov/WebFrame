import { Asset, makeDirectionalAsset } from "valdi_core/src/Asset";
import { Component } from "valdi_core/src/Component";

interface ViewModel {
  ltrAsset: string | Asset;
  rtlAsset: string | Asset;
}

export class DirectionalAsset extends Component<ViewModel> {

  private asset?: Asset;

  onViewModelUpdate(): void {
    this.asset = makeDirectionalAsset(this.viewModel.ltrAsset, this.viewModel.rtlAsset);
  }

  onRender() {
    <view>
      <image src={this.asset}/>
    </view>
  }

}
