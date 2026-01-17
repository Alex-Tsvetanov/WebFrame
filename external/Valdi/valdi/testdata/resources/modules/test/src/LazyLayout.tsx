import { Component } from "valdi_core/src/Component";

interface ViewModel {
  items: any[];
}

export class LazyLayout extends Component<ViewModel> {
  onRender() {
    <layout>
      <scroll>
        {this.onRenderItems()}
      </scroll>
    </layout>
  }
  onRenderItems() {
    for (const item of this.viewModel.items) {
      <view lazyLayout={true} limitToViewport={true} width="40" height="40">
        <view margin="10" id="inner" flexGrow={1}>
          <view width="2" height="2"/>
        </view>
      </view>
    }
  }
}
