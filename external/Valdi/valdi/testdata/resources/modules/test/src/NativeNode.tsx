import { Component } from "valdi_core/src/Component";
import { ElementRef } from "valdi_core/src/ElementRef";
import { NativeNode } from "valdi_tsx/src/NativeNode";
import { View } from "valdi_tsx/src/NativeTemplateElements";

export class NativeNodeComponent extends Component {

  ref = new ElementRef<View>();

  onRender(): void {
    <view>
      <view ref={this.ref} backgroundColor="red"/>
    </view>
  }

  // Called from the C++ test
  receiveNode(receiver: (node: NativeNode | undefined) => void) {
    const node = this.ref.single()!.getNativeNode();
    receiver(node);
  }

}
