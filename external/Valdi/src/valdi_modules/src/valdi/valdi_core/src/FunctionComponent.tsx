import { Component } from 'valdi_core/src/Component';

interface FunctionComponentViewModel {
  render: () => void;
}
export class FunctionComponent extends Component<FunctionComponentViewModel> {
  onRender() {
    this.viewModel.render();
  }
}
