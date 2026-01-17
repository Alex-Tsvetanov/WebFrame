<template>
  <RetainNativeRefs>

  </RetainNativeRefs>
</template>

<script lang="ts">

import { LegacyVueComponent } from 'valdi_core/src/Valdi';
import { ValdiRuntime } from 'valdi_core/src/ValdiRuntime';
import { protectNativeRefs } from 'valdi_core/src/NativeReferences';

declare var runtime: ValdiRuntime;

// @Component
export class RetainNativeRefs extends LegacyVueComponent {

  protectContext(cb: (context: any) => void) {
    const context = this.context;

    protectNativeRefs(this);

    // We return the context so that the unit test can retain it
    cb(runtime.makeOpaque(context));
  }

  makeGlobalCallback(cb: (fn: any) => void) {
    cb(runtime.makeOpaque(() => {
      return 'Nice';
    }));
  }

  makeBytes(cb: (fn: ArrayBuffer) => void) {
    const buffer = new ArrayBuffer(8);
    const bytes = new DataView(buffer);

    for (let i = 0; i < buffer.byteLength; i++) {
      bytes.setUint8(i, i * 2);
    }

    cb(buffer);
  }
};

export function callFn(fn: Function): any {
  return fn();
}

</script>
