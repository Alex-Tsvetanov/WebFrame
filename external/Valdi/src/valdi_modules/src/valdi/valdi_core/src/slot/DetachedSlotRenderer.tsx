import { Component } from '../Component';
import { DetachedSlot, IDetachedSlotRenderer } from './DetachedSlot';

interface ViewModel {
  detachedSlot: DetachedSlot;
}

/**
 * DetachedSlotRenderer is a component that can render a render function
 * sets into a DetachedSlot.
 */
export class DetachedSlotRenderer extends Component<ViewModel> implements IDetachedSlotRenderer {
  onViewModelUpdate(previousViewModel?: ViewModel) {
    if (previousViewModel) {
      previousViewModel.detachedSlot.detachRenderer(this);
    }

    this.viewModel.detachedSlot.attachRenderer(this);
  }

  onDestroy() {
    this.viewModel.detachedSlot.detachRenderer(this);
  }

  onRender() {
    this.viewModel.detachedSlot.onRendererReady(this);
  }
}
