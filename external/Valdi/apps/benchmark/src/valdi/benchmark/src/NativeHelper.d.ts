import { StringMap } from 'coreutils/src/StringMap';

/**
 * @ExportModule
 */

// @ExportModel
export interface FunctionTiming {
  // milliseoncs since epoch in doubles
  enterTime: number;
  leaveTime: number;
}

// @ExportFunction
export function measureStrings(data: string[]): string[];
// @ExportFunction
export function measureStringMaps(data: StringMap<unknown>[]): StringMap<unknown>[];

// @ExportFunction
export function clearNativeFunctionTiming(): void;

// @ExportFunction
export function getNativeFunctionTiming(): FunctionTiming[];
