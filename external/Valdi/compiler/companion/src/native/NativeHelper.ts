import * as ts from 'typescript';
import { getSyntaxKindString } from '../TSUtils';

export enum NumberType {
  Integer,
  Long,
  Float,
}

const INT32_MAX = 2147483647;
const INT32_MIN = -INT32_MAX - 1;

export class NativeCompilerError extends Error {
  constructor(filename: string, sourceFile: ts.SourceFile, node: ts.Node, message: string) {
    const { line, character } = sourceFile.getLineAndCharacterOfPosition(node.getStart());
    const resolvedErrorMessage = `in ${filename} at ${line + 1},${
      character + 1
    }: ${message} of kind ${getSyntaxKindString(node.kind)} at (${line + 1},${
      character + 1
    }) (node content: ${node.getText()})`;

    super(resolvedErrorMessage);
  }
}

export function getNumericalStringTypeFromNumber(numberValue: number): NumberType {
  if (Number.isInteger(numberValue)) {
    if (numberValue > INT32_MAX || numberValue < INT32_MIN) {
      return NumberType.Long;
    } else {
      return NumberType.Integer;
    }
  } else {
    return NumberType.Float;
  }
}

export function getNumericalStringType(value: string): NumberType {
  if (value.includes('.') || value.includes('e')) {
    return NumberType.Float;
  }

  const numberValue = parseFloat(value);

  return getNumericalStringTypeFromNumber(numberValue);
}

export function getModuleName(filePath: string) {
  const fileName = filePath.substring(filePath.lastIndexOf('/') + 1);
  const fileNameNoExtension = fileName.split('.')[0];
  return fileNameNoExtension;
}

export function onError(filename: string, sourceFile: ts.SourceFile, node: ts.Node, message: string): never {
  throw new NativeCompilerError(filename, sourceFile, node, message);
}

export function logNodeInfo(node: ts.Node): void {
  console.log(`kind: ${getSyntaxKindString(node.kind)} text: ${node.getFullText()}`);
}

export function uniqueNameFromNode(moduleName: string, node: ts.Node): string {
  return `tsn_${moduleName}_node_${node.getStart()}_${node.getEnd()}`;
}
