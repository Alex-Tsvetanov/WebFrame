import { TypeConverter } from 'valdi_core/src/TypeConverter';

export interface InputObject {
  left: number;
  right: number;
}

export interface OutputObject {
  value: string;
}

export function makeConverter(): TypeConverter<InputObject, OutputObject> {
  return {
    toIntermediate: (i) => {
      return {value: `${i.left} ${i.right}`}
    },
    toTypeScript: (i) => {
      const components = i.value.split(' ');
      return { left: Number.parseFloat(components[0]), right: Number.parseFloat(components[1]) };
    },
  }
}

export function add(obj: InputObject): number {
  return obj.left + obj.right;
}

export function makeObject(left: number, right: number): InputObject {
  return {left, right};
}