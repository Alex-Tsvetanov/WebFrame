import { IOutputWriter, OutputWriter } from './OutputWriter';

class ObjectiveCImports implements IOutputWriter {
  private imports = new Set<string>();
  private allImports: string[] = [];

  append(importPath: string): boolean {
    if (this.imports.has(importPath)) {
      return false;
    }
    this.imports.add(importPath);

    this.allImports.push(`#import ${importPath}`);

    return true;
  }

  content(): string {
    if (!this.allImports.length) {
      return '';
    }

    const importsStatemenents = this.allImports.join('\n');

    return `\n${importsStatemenents}\n\n`;
  }
}

export class ObjectiveCWriter extends OutputWriter {
  private imports = new ObjectiveCImports();
  private didAddImport = false;

  appendImport(importPath: string): boolean {
    if (!this.didAddImport) {
      this.didAddImport = true;
      this.append(this.imports);
    }

    return this.imports.append(importPath);
  }
}
