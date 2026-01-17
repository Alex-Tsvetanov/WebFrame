export function getArgumentValue(name: string) {
  const index = process.argv.indexOf(name) + 1;
  if (index >= process.argv.length) {
    throw new Error(`Missing associated argument value for ${name}`);
  }
  return process.argv[index];
}
