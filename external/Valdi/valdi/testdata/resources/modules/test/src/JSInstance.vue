<template>
    <JSInstance id="rootView" is-layout="false">
        <Label id="label"/>
        <View>
            <Button id="button1"/>
            <Button id="button2"/>
            <Button id="button3"/>
        </View>
    </JSInstance>
</template>
<class-mapping>
  <Button ios="UIButton" android="com.snap.valdi.views.ValdiButton" />
</class-mapping>

<style>
JSInstance * {
    position: absolute;
    width: 100%;
    height: 100%;
}
</style>

<script lang="ts">

import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class MyView extends LegacyVueComponent {

     onCreate() {
        super.onCreate();

        this.elements.rootView.attributes.color = 'white';
        this.elements.button1.attributes.value = 'Button 1';
        this.elements.button2.attributes.value = 'Button 2';
        this.elements.button3.attributes.value = 'Button 3';
    }

};

</script>
