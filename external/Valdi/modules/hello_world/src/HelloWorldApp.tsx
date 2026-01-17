import { Asset } from 'valdi_core/src/Asset';
import { Component } from 'valdi_core/src/Component';
import { Label, ScrollView } from 'valdi_tsx/src/NativeTemplateElements';
import { Style } from 'valdi_core/src/Style';
import { systemFont } from "valdi_core/src/SystemFont";

import res from '../res';

/**
 * @ViewModel
 * @ExportModel({ ios: 'ValdiStartViewComponentViewModel', android: 'com.snap.valdi.helloworld.StartViewComponentViewModel'})
 */
export interface StartComponentViewModel {}

/**
 * @Context
 * @ExportModel({ios: 'ValdiStartViewComponentContext', android: 'com.snap.valdi.helloworld.StartViewComponentContext'})
 */
export interface StartComponentContext {
  iconImage?: Asset;
}

/**
 * @Component
 * @ExportModel({ios: 'ValdiStartView', android: 'com.snap.valdi.helloworld.StartView'})
 */
export class SampleApps extends Component<
  StartComponentViewModel,
  StartComponentContext
> {
  onCreate(): void {
    console.log('Hello World onCreate!');
  }

  onRender(): void {
    console.log('Hello World onRender!');
    <view backgroundColor='white'>
      <image src={this.context.iconImage as Asset} alignSelf='center' />
      <scroll style={styles.scroll} padding={16}>
        <layout marginTop={100} flexDirection='row' width='100%' minHeight={10}>
          <image src={res.emoji} height='100%' tint='gray' marginRight={10} />
          <label style={styles.title} value='Welcome to Valdi!' font={systemFont(20)}/>
        </layout>
      </scroll>
    </view>;
  }
}

const styles = {
  scroll: new Style<ScrollView>({
    alignItems: 'center',
    height: '100%',
  }),

  title: new Style<Label>({
    color: 'black',
    accessibilityCategory: 'header',
    width: '100%',
  }),
};
