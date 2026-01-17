import { AttributedTextBuilder } from 'valdi_core/src/utils/AttributedTextBuilder';
import { AttributedTextEntryType } from 'valdi_tsx/src/AttributedText';
import 'jasmine/src/jasmine';

describe('AtributedTextBuilder', () => {
  it('can be empty', () => {
    const output = new AttributedTextBuilder().build();
    expect(output).toEqual([]);
  });

  it('can pass multiple strings', () => {
    const output = new AttributedTextBuilder().appendText('Hello').appendText(' ').appendText('World').build();
    expect(output).toEqual([
      AttributedTextEntryType.Content,
      'Hello',
      AttributedTextEntryType.Content,
      ' ',
      AttributedTextEntryType.Content,
      'World',
    ]);
  });

  it('can push font', () => {
    const output = new AttributedTextBuilder().append('Hello ').pushFont('title').append('World').pop().build();
    expect(output).toEqual([
      AttributedTextEntryType.Content,
      'Hello ',
      AttributedTextEntryType.PushFont,
      'title',
      AttributedTextEntryType.Content,
      'World',
      AttributedTextEntryType.Pop,
    ]);
  });

  it('can push text decoration', () => {
    const output = new AttributedTextBuilder()
      .append('Hello ')
      .pushTextDecoration('underline')
      .append('World')
      .pop()
      .build();
    expect(output).toEqual([
      AttributedTextEntryType.Content,
      'Hello ',
      AttributedTextEntryType.PushTextDecoration,
      'underline',
      AttributedTextEntryType.Content,
      'World',
      AttributedTextEntryType.Pop,
    ]);
  });

  it('can push color', () => {
    const output = new AttributedTextBuilder().append('Hello ').pushColor('red').append('World').pop().build();
    expect(output).toEqual([
      AttributedTextEntryType.Content,
      'Hello ',
      AttributedTextEntryType.PushColor,
      'red',
      AttributedTextEntryType.Content,
      'World',
      AttributedTextEntryType.Pop,
    ]);
  });

  it('can append string and attributes', () => {
    const output = new AttributedTextBuilder()
      .append('Hello ', { color: 'red' })
      .append('World', { color: 'green', font: 'title', textDecoration: 'none' })
      .build();

    expect(output).toEqual([
      AttributedTextEntryType.PushColor,
      'red',
      AttributedTextEntryType.Content,
      'Hello ',
      AttributedTextEntryType.Pop,
      AttributedTextEntryType.PushColor,
      'green',
      AttributedTextEntryType.PushFont,
      'title',
      AttributedTextEntryType.PushTextDecoration,
      'none',
      AttributedTextEntryType.Content,
      'World',
      AttributedTextEntryType.Pop,
      AttributedTextEntryType.Pop,
      AttributedTextEntryType.Pop,
    ]);
  });

  it('can nest styles', () => {
    const output = new AttributedTextBuilder()
      .withStyle({ font: 'bold' }, b => {
        b.appendText('Hello ')
          .withStyle({ font: 'italic' }, b => {
            b.append('World');
          })
          .appendText('!');
      })
      .build();

    expect(output).toEqual([
      AttributedTextEntryType.PushFont,
      'bold',
      AttributedTextEntryType.Content,
      'Hello ',
      AttributedTextEntryType.PushFont,
      'italic',
      AttributedTextEntryType.Content,
      'World',
      AttributedTextEntryType.Pop,
      AttributedTextEntryType.Content,
      '!',
      AttributedTextEntryType.Pop,
    ]);
  });
});
