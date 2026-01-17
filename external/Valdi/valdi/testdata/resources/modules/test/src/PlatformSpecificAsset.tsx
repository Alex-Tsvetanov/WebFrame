import { Asset, PlatformAssetOverrides, makePlatformSpecificAsset } from "valdi_core/src/Asset";
import { Component } from "valdi_core/src/Component";

interface ViewModel {
  defaultAsset: string | Asset;
  iOSOverrideAsset: string | Asset;
}

export class PlatformSpecificAsset extends Component<ViewModel> {

  private asset?: Asset;

  onViewModelUpdate(): void {
    const platformOverrides: PlatformAssetOverrides = {
        ios: this.viewModel.iOSOverrideAsset
    };
    this.asset = makePlatformSpecificAsset(this.viewModel.defaultAsset, platformOverrides);
  }

  onRender() {
    <view>
      <image src={this.asset}/>
    </view>
  }

}
