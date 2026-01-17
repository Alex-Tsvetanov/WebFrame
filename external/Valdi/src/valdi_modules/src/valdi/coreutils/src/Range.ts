export interface Range {
  min: number;
  max: number;
}

// @ExportModel({ios: 'SCCCoreUtilsMediaTimeRange', android: 'com.snap.valdi.coreutils.MediaTimeRange'})
export interface MediaTimeRange {
  startMs: number;
  durationMs: number;
}
