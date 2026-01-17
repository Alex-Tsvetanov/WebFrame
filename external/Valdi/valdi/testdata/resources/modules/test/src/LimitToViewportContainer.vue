<template>
    <LimitToViewportContainer overflow="scroll" flex-direction="column" is-layout="false">
        <View id="container" for-each="{{ entry of viewModel.entries }}" limit-to-viewport="true" width="100" height="100">
            <View id="child" width="100" height="100"/>
        </View>
    </LimitToViewportContainer>
</template>
<script lang="ts">
// no-op
</script>
