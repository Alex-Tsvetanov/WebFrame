const escaped = '/$^+.()=!|';

export function globToRegex(glob: string) {
  let regex = '';
  for (let i = 0; i < glob.length; i++) {
    const c = glob[i];
    if (escaped.includes(c)) {
      regex += '\\' + c;
    } else if (c === '*') {
      const c1 = glob[i + 1];
      if (c1 === '*') {
        regex += '(?:.*)';
        i++;
      } else {
        regex += '([^/]*)';
      }
    } else {
      regex += c;
    }
  }
  regex = '^' + regex + '$';
  return new RegExp(regex);
}
