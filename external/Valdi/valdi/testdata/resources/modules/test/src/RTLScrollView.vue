<template>

    <RTLScrollView is-layout="false">
        <ScrollView id="container" width="10" height="10" padding-left="1" padding-right="2" overflow="scroll" flex-direction="row">
            <View id="entries" width="10" margin-left="3" margin-right="4" height="10" for-each="{{ entry of viewModel.entries }}" custom-tag="{{ entry }}"/>
        </ScrollView>
    </RTLScrollView>

</template>
<script lang="ts">
// no-op
</script>
