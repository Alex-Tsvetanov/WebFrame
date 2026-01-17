<template>
    <RenderIf is-layout="false">
        <Button>Button 1</Button>
        <Button render-if="{{ viewModel?.renderButton2 }}">Button 2</Button>
        <Button render-if="{{ viewModel?.renderButton3 }}">Button 3</Button>
    </RenderIf>
</template>
<class-mapping>
  <Button ios="UIButton" android="com.snap.valdi.views.ValdiButton" />
</class-mapping>
<script lang="ts">
// no-op
</script>
