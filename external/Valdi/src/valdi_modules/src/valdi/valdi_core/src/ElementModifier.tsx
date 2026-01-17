import { View } from 'valdi_tsx/src/NativeTemplateElements';
import { Component } from './Component';
import { ElementRef } from './ElementRef';
export interface ViewModel extends View {}

/**
 * ElementModifier provides an easy way to modify a View's appearance / behaviour without creating a physical
 * container view, or replicating view attributes in Component view models and pass them down through layers
 * of components. Example usage:
 *
 * <ElementModifer opacity={0.5}>
 *  { * <MyCustomComponent/> *}
 * </ElementModifer>
 *
 * This will set the opacity of the final view rendered by MyCustomComponent.
 * ElementModifier can be nested, e.g.
 *
 * <ElementModifer scaleX={2}>
 *  <ElementModifer opacity={0.5}>
 *    { * <MyCustomComponent/> *}
 *  </ElementModifer>
 * </ElementModifier>
 *
 * ElementModifier achieve this by aquiring the elementRef from your component and directly set its attributes.
 * Be careful setting duplicate attributes may result in unexpected results
 */
//These are the attributes that's injected by parent container. Hard code the attributes here
// until we figure more idomatic way to exclude these attributes.
const INJECTED_ATTRS = ['children'];
export class ElementModifier extends Component<ViewModel> {
  elementRef = new ElementRef<View>();

  onViewModelUpdate(): void {
    for (const key in this.viewModel) {
      const viewModelKey = key as keyof View;
      if (typeof viewModelKey === 'string' && INJECTED_ATTRS.includes(viewModelKey)) {
        continue;
      }
      this.elementRef.setAttribute(viewModelKey, this.viewModel[viewModelKey]);
    }
  }

  onRender(): void {
    <slot ref={this.elementRef} />;
  }
}
