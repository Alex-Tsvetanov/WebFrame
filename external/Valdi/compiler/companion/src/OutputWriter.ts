function makeIndent(indent: number): string {
  let out = '';

  for (let i = 0; i < indent; i++) {
    out += '  ';
  }

  return out;
}

export interface IOutputWriter {
  content(): string;
}

type OutputWriterContent = string | IOutputWriter;

function outputContent(contents: OutputWriterContent[]): string {
  let output = '';

  for (const content of contents) {
    if ((content as IOutputWriter).content) {
      output += (content as IOutputWriter).content();
    } else {
      output += content;
    }
  }

  return output;
}

class IndentedContent implements IOutputWriter {
  constructor(readonly written: OutputWriterContent[], readonly indentation: string) {}

  content(): string {
    let content = outputContent(this.written);
    const lines = content.split('\n');

    const processedLines = lines.map((line) => (line ? this.indentation + line : line));
    return processedLines.join('\n');
  }
}

interface Indentation {
  indentation: string;
  previouslyWritten: OutputWriterContent[];
}

export class OutputWriter implements IOutputWriter {
  private written: OutputWriterContent[] = [];
  private indentations: Indentation[] = [];

  get writtenLength(): number {
    return this.written.length;
  }

  append(str: string | IOutputWriter) {
    this.written.push(str);
  }

  content(): string {
    return outputContent(this.written);
  }

  beginIndent(indentation: string) {
    const written = this.written;
    this.written = [];
    this.indentations.push({ indentation, previouslyWritten: written });
  }

  endIndent() {
    const indentation = this.indentations.pop();
    if (!indentation) {
      throw new Error('Unbalanced beginIndent/endIndent');
    }
    const writtenWithIdentation = this.written;
    this.written = indentation.previouslyWritten;
    if (writtenWithIdentation.length) {
      this.append(new IndentedContent(writtenWithIdentation, indentation.indentation));
    }
  }

  withIndentation(indentation: string, cb: () => void) {
    this.beginIndent(indentation);
    cb();
    this.endIndent();
  }
}
