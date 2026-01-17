export function isUndefined(val: any): val is undefined {
  return typeof val === 'undefined';
}
