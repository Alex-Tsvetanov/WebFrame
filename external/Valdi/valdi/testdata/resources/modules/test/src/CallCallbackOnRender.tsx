import { Component } from "valdi_core/src/Component";

interface ViewModel {
  onRender(): void;
}

export class CallCallbackOnRender extends Component<ViewModel> {

  onRender() {
    this.viewModel.onRender();
    <view/>;
  }
}
