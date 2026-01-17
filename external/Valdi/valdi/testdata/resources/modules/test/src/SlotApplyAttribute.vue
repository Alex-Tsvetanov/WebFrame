<template>
    <SlotApplyAttribute is-layout="false">
        <Slot ref="{{ this.slotRef }}" />
    </SlotApplyAttribute>
</template>

<script lang="ts">
import { ElementRef } from "valdi_core/src/ElementRef";
import { LegacyVueComponent } from "valdi_core/src/Valdi";

// @Component
export class MyComponent extends LegacyVueComponent<any> {

  slotRef = new ElementRef();

  onViewModelUpdate() {
      this.slotRef.setAttribute('text', this.viewModel && this.viewModel.valueInSlot);
  }
}
</script>