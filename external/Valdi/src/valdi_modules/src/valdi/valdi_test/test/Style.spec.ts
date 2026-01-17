import { NativeStyle, Style } from 'valdi_core/src/Style';
import { enumeratePropertyList, PropertyList } from 'valdi_core/src/utils/PropertyList';
import 'jasmine/src/jasmine';

function propertyListToObject(attributes: PropertyList): NativeStyle {
  const out: any = {};
  enumeratePropertyList(attributes, (key, value) => {
    out[key] = value;
  });
  return out;
}

function styleToNative<T>(style: Style<T>): any {
  return style.toNative(propertyListToObject);
}

describe('Style', () => {
  it('can convert to native', () => {
    const style = new Style({ hello: 'world', nice: 42 });
    const native = styleToNative(style);

    expect(native).toEqual({ hello: 'world', nice: 42 });
  });

  it('convert to native only once', () => {
    const style = new Style({ hello: 'world', nice: 42 });
    const native = styleToNative(style);
    const native2 = styleToNative(style);

    expect(native).toBeTruthy();
    expect(native).toBe(native2);
  });

  it('can be extended with new attributes', () => {
    const style = new Style({ hello: 'world' });
    const style2 = style.extend({ nice: 42 });

    const native = styleToNative(style);
    const native2 = styleToNative(style2);

    expect(native).toEqual({ hello: 'world' });
    expect(native2).toEqual({ hello: 'world', nice: 42 });
  });

  it('can override attributes when extending', () => {
    const style = new Style({ hello: 'world', nice: 12 });
    const style2 = style.extend({ nice: 42 });

    const native = styleToNative(style);
    const native2 = styleToNative(style2);

    expect(native).toEqual({ hello: 'world', nice: 12 });
    expect(native2).toEqual({ hello: 'world', nice: 42 });
  });

  it('can merge styles', () => {
    const style = new Style({ hello: 'world', nice: 12 });
    const style2 = new Style({ nice: 42 });
    const style3 = Style.merge(style, style2);
    const style4 = Style.merge(style, style2, style3);
    const style5 = Style.merge(style, style2, style3, style4);

    const native = styleToNative(style);
    const native2 = styleToNative(style2);
    const native3 = styleToNative(style3);
    const native4 = styleToNative(style4);
    const native5 = styleToNative(style5);

    expect(native).toEqual({ hello: 'world', nice: 12 });
    expect(native2).toEqual({ nice: 42 });
    expect(native3).toEqual({ hello: 'world', nice: 42 });
    expect(native4).toEqual({ hello: 'world', nice: 42 });
    expect(native5).toEqual({ hello: 'world', nice: 42 });
  });
});
