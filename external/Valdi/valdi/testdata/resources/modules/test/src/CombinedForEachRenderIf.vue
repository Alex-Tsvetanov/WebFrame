<template>
    <CombinedForEachRenderIf is-layout="false">
        <View for-each="{{ card of viewModel.cards }}" render-if="{{ card.title.length > 0 }}">
            <Label>{{ card.title }}</Label>
            <Label render-if="{{ card.subtitle.length > 0 }}">{{ card.subtitle }}</Label>
        </View>
        <Button>Button 1</Button>
    </CombinedForEachRenderIf>
</template>
<class-mapping>
  <Button ios="UIButton" android="com.snap.valdi.views.ValdiButton" />
</class-mapping>
<script lang="ts">
// no-op
</script>
