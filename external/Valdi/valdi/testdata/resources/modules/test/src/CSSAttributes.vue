<template>
    <CSSAttributes class="main    alsomain" is-layout="false">
        <Label>Title</Label>
        <View id="container" color="{{ viewModel?.containerColor ?? 'black' }}">
            <Button>Button 1</Button>
            <Button>Button 2</Button>
            <Button>Button 3</Button>
        </View>
    </CSSAttributes>
</template>
<class-mapping>
  <Button ios="UIButton" android="com.snap.valdi.views.ValdiButton" />
</class-mapping>

<style>
.main {
    color: white;
}

.alsomain {
    align-items: center;
}

Label {
    font: title;
}

Button {
    dummyHeight: 40;
}

Button:last-child {
    dummyHeight: 45.0;
}

Button:first-child {
    dummyHeight: 35;
}

View[color="black"] {
    justify-content: space-between;
}

</style>
<script lang="ts">
// no-op
</script>
