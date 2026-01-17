<template>
    <LocalComponentInRemoteComponent is-layout="false">
        <View>
            <Label>This part is remote</Label>
        </View>
        <View>
            <Label>This part is local</Label>
            <BasicViewTree/>
        </View>
    </LocalComponentInRemoteComponent>
</template>
<script lang="ts">
// no-op
</script>
