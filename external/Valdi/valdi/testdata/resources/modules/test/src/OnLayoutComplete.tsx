import { ElementFrame } from "valdi_tsx/src/Geometry";
import { Component } from "valdi_core/src/Component";
import { ElementRef } from "valdi_core/src/ElementRef";

interface ViewModel {
  onLayoutComplete(rootFrame: ElementFrame, childFrame: ElementFrame): void;
  childSize: number;
}

export class OnLayoutComplete extends Component<ViewModel> {
  private rootRef = new ElementRef();
  private childRef = new ElementRef();

  onRender() {
    this.renderer.onLayoutComplete(() => {
      const rootFrame = this.rootRef.single()!.frame;
      const childFrame = this.childRef.single()!.frame;

      this.viewModel.onLayoutComplete(rootFrame, childFrame);
    });

    <view ref={this.rootRef} width={100} height={100}>
      <layout ref={this.childRef} left={25} top={25} width={this.viewModel.childSize} height={this.viewModel.childSize}/>
    </view>
  }
}