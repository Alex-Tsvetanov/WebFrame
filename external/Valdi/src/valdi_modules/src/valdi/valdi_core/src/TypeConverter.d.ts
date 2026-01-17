export interface TypeConverter<TSType, IntermediateType> {
  /**
   * Convert the value from TypeScript into an intermediate type
   * that can be passed to the native code.
   */
  toIntermediate(value: TSType): IntermediateType;

  /**
   * Conveert the value provided as an intermediate type from the
   * native type into a value that can be used by the TypeScript code
   */
  toTypeScript(value: IntermediateType): TSType;
}
