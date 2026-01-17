<template>
    <BidirectionalAttributes is-layout="false">
        <View id="container" for-each="{{ label of viewModel.labels }}">
            <Label id="label">{{ label }}</Label>
        </View>
    </BidirectionalAttributes>
</template>
<script lang="ts">
// no-op
</script>
