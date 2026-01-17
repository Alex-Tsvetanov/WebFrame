import { fs } from 'file_system/src/FileSystem';
import { CoverageMapData } from './IstanbulIibCoverage';

declare const __coverage__: CoverageMapData;

export class CodeCoverageManager {
  private ignoredErrorType = 'No item named';

  constructor(private readonly codeCoverageResultFile: string) {}

  collectAndSaveCoverageInfo() {
    console.log('[CodeCoverage]: start to collect coverage information');
    fs.writeFileSync(`${this.codeCoverageResultFile}`, JSON.stringify(__coverage__));
    console.log('[CodeCoverage]: coverage information was collected');
  }
}
