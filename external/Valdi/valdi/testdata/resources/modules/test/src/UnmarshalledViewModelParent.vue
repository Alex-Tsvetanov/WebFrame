<template is-layout="false">
    <UnmarshalledViewModelParent>
        <UnmarshalledViewModelChild id="child"/>

    </UnmarshalledViewModelParent>
</template>

<script lang="ts">

import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class MyComponent extends LegacyVueComponent {

    childViewModel: any;

    onCreate() {
        super.onCreate();

        this.childViewModel = {
            count: 1
        };

        this.elements.child.attributes.viewModel = this.childViewModel;
    }

    incrementCountOfChildViewModel() {
        this.childViewModel.count += 1;
    }

}

</script>
