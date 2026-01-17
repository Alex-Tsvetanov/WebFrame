import { StatefulComponent } from 'valdi_core/src/Component';
import { CSSValue, Layout } from 'valdi_tsx/src/NativeTemplateElements';
import { ElementRef } from './ElementRef';

// Has to be provided with enough attributes to lay out from outside-in
interface ViewModel {
  width?: CSSValue;
  height?: CSSValue;
  estimatedWidth?: number;
  estimatedHeight?: number;
  top?: CSSValue;
  right?: CSSValue;
  bottom?: CSSValue;
  left?: CSSValue;
  /** Default to true unless estimated size is set */
  lazyLayout?: boolean;
  elementRef?: ElementRef<Layout>;
}

interface State {
  wasEverVisible: boolean;
}

/**
 * The Lazy component renders its slot when its layout node becomes visible for the first time.
 * The component must either be provided a set of attributes that allows the underlying node
 * to have a size without rendering the children, or the estimatedWidth/estimatedHeight
 * attributes should be passed such that the node can have a placeholder size while waiting
 * for the children to render. The use of "estimatedWidth" and "estimatedHeight" makes the
 * Lazy component not use "lazyLayout", as the node can be resized once the children are rendered.
 */
export class Lazy extends StatefulComponent<ViewModel, State> {
  state: State = {
    wasEverVisible: false,
  };

  renderContents() {
    if (this.state.wasEverVisible) {
      <slot />;
    }
  }

  private onVisibilityChanged = (isVisible: boolean) => {
    if (this.state.wasEverVisible || !isVisible) {
      return;
    }
    this.setState({ wasEverVisible: true });
  };

  onRender() {
    const viewModel = this.viewModel;
    const estimatedWidth = viewModel.estimatedWidth;
    const estimatedHeight = viewModel.estimatedHeight;
    const lazyLayout = viewModel.lazyLayout ?? (estimatedWidth === undefined && estimatedHeight === undefined);

    <layout
      width={viewModel.width}
      height={viewModel.height}
      left={viewModel.left}
      top={viewModel.top}
      right={viewModel.right}
      bottom={viewModel.bottom}
      estimatedWidth={estimatedWidth}
      estimatedHeight={estimatedHeight}
      lazyLayout={lazyLayout}
      onVisibilityChanged={this.onVisibilityChanged}
      ref={viewModel.elementRef}
    >
      {this.renderContents()}
    </layout>;
  }
}
