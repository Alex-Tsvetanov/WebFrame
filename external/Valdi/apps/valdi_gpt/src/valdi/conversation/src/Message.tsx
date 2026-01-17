import { Component } from 'valdi_core/src/Component';
import { Style } from 'valdi_core/src/Style';
import { systemFont } from 'valdi_core/src/SystemFont';
import { Label } from 'valdi_tsx/src/NativeTemplateElements';

export interface MesssageViewModel {
  text: string;
  outbound: boolean;
}

export class Message extends Component<MesssageViewModel> {
  onRender(): void {
    const direction = this.viewModel.outbound ? 'row-reverse' : 'row';
    <layout width="100%" flexDirection={direction}>
      <view backgroundColor={this.viewModel.outbound ? '#e3e3e3' : 'white'} borderRadius={6} padding={8} width="80%">
        <label
          numberOfLines={0}
          style={this.viewModel.outbound ? styles.outbound : styles.inbound}
          value={this.viewModel.text}
          textAlign="left"
        />
      </view>
    </layout>;
  }
}

const styles = {
  outbound: new Style<Label>({
    color: 'black',
    textAlign: 'right',
    backgroundColor: '#e3e3e3',
    borderRadius: 6,
    font: systemFont(18),
  }),

  inbound: new Style<Label>({
    color: 'black',
    backgroundColor: 'white',
    textAlign: 'left',
    borderRadius: undefined,
    font: systemFont(18),
  }),
};
