<template>
    <RemoteScriptInLocalComponent is-layout="false">
        <Label>This is remote</Label>
        <Label>{{ this.getText() }}</Label>
    </RemoteScriptInLocalComponent>
</template>

<script lang="ts">

import { add } from './RemoteTS';
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class LocalComponent extends LegacyVueComponent {

    getText(): string {
        return 'From remote: ' + add(1, 2);
    }

}
</script>
