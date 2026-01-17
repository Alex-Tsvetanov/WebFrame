<template>
    <AssetLoading is-layout="false">
        <ImageView id="imageView" src="{{ viewModel.src }}"/>
    </AssetLoading>
</template>

<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';
import { addAssetLoadObserver, AssetOutputType, AssetSubscription } from "valdi_core/src/Asset";

// @Component
export class MyComponent extends LegacyVueComponent {
    private subscription?: AssetSubscription;

    loadAsset(src: string, callback: (asset: unknown, error: string | undefined) => void): void {
        this.subscription = addAssetLoadObserver(src, callback, AssetOutputType.DUMMY)
    }

    unloadAsset() {
        this.subscription?.unsubscribe();
    }
}

</script>
