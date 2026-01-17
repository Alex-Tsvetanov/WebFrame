export interface NativeCompilerOptions {
  optimizeSlots: boolean;
  optimizeVarRefs: boolean;

  // Smaller code size and faster
  optimizeNullChecks?: boolean;
  mergeSetProperties?: boolean;
  foldConstants?: boolean;
  optimizeVarRefLoads?: boolean;
  optimizeAssignments?: boolean;
  inlinePropertyCache?: boolean;
  enableIntrinsics?: boolean;

  // Smaller code size but slower
  noinlineRetainRelease?: boolean;
  mergeReleases?: boolean;
  autoRelease?: boolean;
}
