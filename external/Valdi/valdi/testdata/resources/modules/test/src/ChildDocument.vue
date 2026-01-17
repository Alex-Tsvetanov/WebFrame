<template>
    <ChildDocument is-layout="false">
        <DynamicAttributes view-model="{{ viewModel?.childViewModel }}"/>
        <Button>Button 1</Button>
    </ChildDocument>
</template>
<class-mapping>
  <Button ios="UIButton" android="com.snap.valdi.views.ValdiButton" />
</class-mapping>
<script lang="ts">
// no-op
</script>
