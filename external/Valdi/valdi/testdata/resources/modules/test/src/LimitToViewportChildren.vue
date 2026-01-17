<template>
    <LimitToViewportChildren overflow="scroll" flex-direction="column" is-layout="false">
        <View id="container" for-each="{{ entry of viewModel.entries }}" width="100" height="100">
            <View id="parent" width="50" height="50">
                <View id="child" width="10" height="10" limit-to-viewport="true"/>
            </View>
        </View>
    </LimitToViewportChildren>
</template>
<script lang="ts">
// no-op
</script>
