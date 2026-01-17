export class GeneratedFileHeader {
  static generateMultilineComment(comment: string): string {
    const commentLines = comment
      .split('\n')
      .map((line) => ` * ${line}`)
      .join('\n');
    return `/**\n${commentLines}\n */\n`;
  }
}
