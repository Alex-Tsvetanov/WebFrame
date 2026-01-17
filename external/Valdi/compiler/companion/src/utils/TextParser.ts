export const enum CharCode {
  space = 32,
  newline = 10,
  doubleQuote = 34,
  dollar = 36,
  singleQuote = 39,
  semiColon = 59,
  underscore = 95,
}

export class TextParser {
  position = 0;
  private currentLine = 0;
  private currentLineGlobalPosition = 0;

  constructor(readonly text: string) {}

  debugPositionString(): string {
    return `line ${this.currentLine + 1}, column: ${this.position - this.currentLineGlobalPosition + 1}`;
  }

  private onError(message: string): never {
    throw new Error(`At ${this.debugPositionString()}: ${message}`);
  }

  isAtEnd(): boolean {
    return this.position >= this.text.length;
  }

  parse(term: string): void {
    if (!this.tryParse(term)) {
      this.onError(`Expecting '${term}'`);
    }
  }

  parseQuotedString(): string {
    if (this.tryParse('"')) {
      return this.parseUntilCharCode(CharCode.doubleQuote);
    } else {
      this.parse("'");
      return this.parseUntilCharCode(CharCode.singleQuote);
    }
  }

  parseUntilCharCode(charCode: number): string {
    let startPosition = this.position;
    let endPosition = startPosition;
    let foundCharCode = false;

    while (endPosition < this.text.length) {
      const code = this.text.charCodeAt(endPosition);
      endPosition++;
      if (code === charCode) {
        foundCharCode = true;
        break;
      }
    }

    this.position = endPosition;

    return this.text.substring(startPosition, foundCharCode ? endPosition - 1 : endPosition);
  }

  peek(term: string): boolean {
    this.consumeWhitespaces();

    let position = this.position;
    if (this.tryParse(term)) {
      this.position = position;
      return true;
    }
    return false;
  }

  tryParse(term: string): boolean {
    this.consumeWhitespaces();

    for (let i = 0; i < term.length; i++) {
      if (term.charCodeAt(i) !== this.text.charCodeAt(this.position + i)) {
        return false;
      }
    }

    this.position += term.length;
    return true;
  }

  private isIdentifier(position: number): boolean {
    const code = this.text.charCodeAt(position);
    return (
      (code > 47 && code < 58) ||
      (code > 64 && code < 91) ||
      (code > 96 && code < 123) ||
      code === CharCode.underscore ||
      code === CharCode.dollar
    );
  }

  isAtIdentifier(): boolean {
    this.consumeWhitespaces();
    return this.isIdentifier(this.position);
  }

  parseIdentifier(): string {
    this.consumeWhitespaces();
    let startPosition = this.position;
    let endPosition = startPosition;

    for (;;) {
      if (this.isIdentifier(endPosition)) {
        endPosition++;
      } else {
        break;
      }
    }

    if (startPosition === endPosition) {
      this.onError('Not an identifier');
    }

    this.position = endPosition;

    return this.text.substring(startPosition, endPosition);
  }

  skipCurrentLine(): void {
    while (!this.isAtEnd()) {
      const code = this.text.charCodeAt(this.position);
      if (code === CharCode.newline) {
        this.position++;
        this.currentLine++;
        this.currentLineGlobalPosition = this.position;
        return;
      } else {
        this.position++;
      }
    }
  }

  private consumeWhitespaces() {
    for (;;) {
      let code = this.text.charCodeAt(this.position);
      if (code === CharCode.space) {
        this.position++;
      } else if (code === CharCode.newline) {
        this.position++;
        this.currentLine++;
        this.currentLineGlobalPosition = this.position;
      } else {
        break;
      }
    }
  }
}
