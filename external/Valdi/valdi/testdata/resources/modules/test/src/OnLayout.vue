<template>
    <OnLayout align-items="center" justify-content="space-around" is-layout="false">
        <View width="50%" height="50%" on-layout="{{ this.viewModel.layoutCallback }}"/>
    </OnLayout>
</template>
<script lang="ts">
// no-op
</script>
