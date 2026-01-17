import { Component } from 'valdi_core/src/Component';
import { AttributedTextBuilder } from 'valdi_core/src/utils/AttributedTextBuilder';

export class TextAttribute extends Component {

  onRender() {
    const attributedTextBuilder = new AttributedTextBuilder();

    attributedTextBuilder.append('Hello');
    attributedTextBuilder.withStyle({
      color: 'red',
      font: 'title',
      textDecoration: 'underline'
    }, (b) => {
      b.appendText(' ')
      .appendText('World')
    });

    attributedTextBuilder.pushColor('blue');
    attributedTextBuilder.appendText('!');
    attributedTextBuilder.pushColor('green');
    attributedTextBuilder.appendText('?');
    attributedTextBuilder.pop();
    attributedTextBuilder.appendText('!');
    attributedTextBuilder.pop();

    <view>
      <label value={attributedTextBuilder.build()}/>;
    </view>
  }

}
