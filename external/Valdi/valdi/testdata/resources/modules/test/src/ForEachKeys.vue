<template>
  <ForEachKeys is-layout="false">
      <ForEachKeyItem for-each="{{ index in viewModel.items }}" view-model="{{ viewModel.items[index] }}" key="{{ viewModel.useKeys ? viewModel.items[index].id : index }}"/>
  </ForEachKeys>
</template>
<script lang="ts">
// no-op
</script>
