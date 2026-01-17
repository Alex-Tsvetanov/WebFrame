<template>
    <RemoteAsyncLoadComponent>
        <View render-if="{{ state?.isError }}">
            <Label>Error occured</Label>
        </View>
        <View render-else-if="{{ !state?.isLoaded }}">
            <Label>We are not loaded</Label>
        </View>
        <RemoteComponent render-else />
    </RemoteAsyncLoadComponent>
</template>

<script lang="ts">

import { LegacyVueComponent, isModuleLoaded, loadModule } from 'valdi_core/src/Valdi';

interface ViewModel {
    moduleName: string;
}

interface State {
    isError: boolean;
    isLoaded: boolean;
}

// @Component
export class RemoteAsyncLoadComponent extends LegacyVueComponent<ViewModel, State> {

    onCreate() {
        this.state = {
            isError: false,
            isLoaded: isModuleLoaded(this.viewModel.moduleName)
        };

        if (!this.state.isLoaded) {
            loadModule(this.viewModel.moduleName, (error) => {
                if (!error) {
                    this.setState({
                        isLoaded: true
                    });
                } else {
                    this.setState({
                        isError: true
                    });
                }
            });
        }
    }

}

</script>
