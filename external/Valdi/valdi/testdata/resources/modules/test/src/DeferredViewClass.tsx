import { Component } from "valdi_core/src/Component";
import { ViewFactory } from "valdi_tsx/src/ViewFactory";

interface ViewModel {
  iosClassName?: string;
  androidClassName?: string;
  viewFactory?: ViewFactory;
}

export class DeferredViewClass extends Component<ViewModel> {

  onRender() {
    <view>
      {this.renderInner()}
    </view>
  }

  private renderInner() {
    if (this.viewModel.iosClassName && this.viewModel.androidClassName) {
      <custom-view iosClass={this.viewModel.iosClassName} androidClass={this.viewModel.androidClassName}/>
    } else if (this.viewModel.viewFactory) {
      <custom-view viewFactory={this.viewModel.viewFactory}/>
    }
  }

}
