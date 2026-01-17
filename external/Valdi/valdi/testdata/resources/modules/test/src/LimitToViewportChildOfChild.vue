<template>

    <LimitToViewportChildOfChild limit-to-viewport="true" is-layout="false">
        <ChildDocument :childViewModel="{padding: 10, title: 'Hello', subtitleHeight: 45.5}"/>
    </LimitToViewportChildOfChild>

</template>

<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';
import { IRenderedElement } from "../../valdi_core/src/IRenderedElement";

function makeFullsize(element: IRenderedElement) {
  element.setAttribute('width', '100%');
  element.setAttribute('height', '100%');
  element.setAttribute('position', 'absolute');

  element.children.forEach(makeFullsize);
}

// @Component
export class MyComponent extends LegacyVueComponent {
    onRender() {
        super.onRender();

        for (const element of this.rootElements) {
          element.children.forEach(makeFullsize);
        }
    }
}

</script>