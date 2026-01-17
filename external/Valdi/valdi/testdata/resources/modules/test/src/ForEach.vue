<template>
    <ForEach is-layout="false">
        <!-- <View render-if="{{ 'false' === false }}"> -->
            <View for-each="{{ card of ((viewModel && viewModel.cards) || []) }}" color="white" z-index="{{ card.zIndex }}">
                <Label>{{ card.title }}</Label>
                <Label>{{ card.subtitle }}</Label>
            </View>
        <!-- </View> -->
        <Button>Button 1</Button>
    </ForEach>
</template>
<class-mapping>
  <Button ios="UIButton" android="com.snap.valdi.views.ValdiButton" />
</class-mapping>
<script lang="ts">
// no-op
</script>
