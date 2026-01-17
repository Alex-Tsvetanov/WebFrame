import { IOutputWriter, OutputWriter } from '../OutputWriter';

export class NativeCodeWriter extends OutputWriter {
  private static INDENT = '    ';

  appendWithNewLine(str: string | IOutputWriter | undefined = undefined): void {
    if (str !== undefined) {
      this.append(str);
    }
    this.append('\n');
  }

  appendAssignment(variable: string, str: string): void {
    this.startAssignment(variable);
    this.append(str);
    this.endAssignment();
  }

  appendLabel(label: string) {
    this.appendWithNewLine(`${label}:`);
  }

  appendAssignmentWithFunctionCall(variable: string, func: string, args: string[]) {
    this.startAssignment(variable);
    this.appendFunctionCall(func, args, false);
    this.endAssignment();
  }

  appendFunctionCall(func: string, args: Array<string>, endStatement: boolean = true) {
    this.startFunctionCall(func);
    this.appendParameterList(args);
    this.endFunctionCall(endStatement);
  }

  private startAssignment(variable: string) {
    this.append(`${variable} = `);
  }

  private endAssignment() {
    this.appendWithNewLine(';');
  }

  private startFunctionCall(funcName: string) {
    this.append(`${funcName}(`);
  }

  private endFunctionCall(endStatement: boolean = false) {
    this.append(')');
    if (endStatement) {
      this.appendWithNewLine(';');
    }
  }

  appendParameterList(args: Array<string>) {
    if (args.length == 0) {
      return;
    }

    this.append(args.join(', '));
  }

  appendGoto(label: string) {
    this.appendWithNewLine(`goto ${label};`);
  }

  beginIfStatement() {
    this.append(`if (`);
  }

  endEndifStatement() {
    this.append(`)`);
  }

  beginInitialization() {
    this.beginScope();
  }

  endInitialization() {
    this.endScope(false);
    this.appendWithNewLine(';');
  }

  beginScope() {
    this.appendWithNewLine(` {`);
    this.beginIndent(NativeCodeWriter.INDENT);
  }

  endScope(withNewLine: boolean = true) {
    this.endIndent();
    this.append('}');
    if (withNewLine) {
      this.appendWithNewLine();
    }
  }

  beginComment() {
    this.appendWithNewLine(`/*`);
    this.beginIndent(NativeCodeWriter.INDENT);
  }

  endComment() {
    this.endIndent();
    this.appendWithNewLine(`*/`);
  }
}
