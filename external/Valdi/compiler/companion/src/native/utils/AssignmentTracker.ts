import { NativeCompilerBuilderAtomID, NativeCompilerBuilderVariableID } from '../builder/INativeCompilerBuilder';

interface Assignment {
  readonly object: NativeCompilerBuilderVariableID;
  readonly property: NativeCompilerBuilderAtomID | NativeCompilerBuilderVariableID;
}

export class AssignmentTracker {
  get assignments(): readonly Assignment[] {
    return this._assignemnts;
  }

  private _assignemnts: Assignment[] = [];

  onGetProperty(object: NativeCompilerBuilderVariableID, property: NativeCompilerBuilderAtomID): void {
    this._assignemnts.push({ object, property });
  }

  onGetPropertyValue(object: NativeCompilerBuilderVariableID, property: NativeCompilerBuilderVariableID): void {
    this._assignemnts.push({ object, property });
  }
}
