<template>
    <Margin align-items="center" justify-content="space-around" is-layout="false">
        <View id="parent">
            <View id="child" margin="{{ viewModel?.margin }}" margin-left="{{ viewModel?.marginLeft }}" margin-right="{{ viewModel?.marginRight }}" margin-top="{{ viewModel?.marginTop }}" margin-bottom="{{ viewModel?.marginBottom }}" width="50" height="50"/>
        </View>
    </Margin>
</template>

<script lang="ts">
// no-op
</script>
