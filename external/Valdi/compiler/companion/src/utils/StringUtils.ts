export function trimAllLines(str: string): string {
  const outLines: string[] = [];

  for (const line of str.split('\n')) {
    const trimmed = line.trim();

    if (trimmed) {
      outLines.push(trimmed);
    }
  }

  return outLines.join('\n');
}
