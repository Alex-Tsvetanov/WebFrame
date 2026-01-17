const hasOwn = {}.hasOwnProperty;

/**
 * classnames takes a list of arguments and constructs a class name string.
 * Arguments may be strings, numbers, arrays or objects. Strings and numbers
 * are applied directly, arrays are recursively called per item, and objects
 * have their keys applied as classnames only if the value evalutes to true.
 *
 * e.g.,
 * import cx from 'common/src/classnames'
 * class="{{ cx('button', { 'enabled': viewModel.enabled }) }}" results in 'button enabled'
 * if viewModel.enabled is true.
 */
export function classNames(...args: any[]): string {
  const classes: string[] = [];
  for (const arg of args) {
    if (!arg) {
      continue;
    }
    const type = typeof arg;
    if (type === 'string' || type === 'number') {
      classes.push(arg);
    } else if (Array.isArray(arg) && arg.length) {
      // eslint-disable-next-line prefer-spread
      const inner = classNames.apply(null, arg);
      if (inner) {
        classes.push(inner);
      }
    } else if (type === 'object') {
      for (const key in arg) {
        if (hasOwn.call(arg, key) && arg[key]) {
          classes.push(key);
        }
      }
    }
  }
  return classes.join(' ');
}
