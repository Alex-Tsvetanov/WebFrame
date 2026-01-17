<template>
    <RemoteScriptInLocalComponent is-layout="false">
        <Label>This is local</Label>
        <Label>{{ this.getText() }}</Label>
    </RemoteScriptInLocalComponent>
</template>

<script lang="ts">

import { add } from 'remote/src/RemoteTS';
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class LocalComponent extends LegacyVueComponent {

    getText(): string {
        return 'From remote: ' + add(42, 42);
    }

}
</script>
