import { StringMap } from 'coreutils/src/StringMap';

interface LogMetadataProviderInfo {
  name: string;
  provider: LogMetadataProvider;
}

export interface DumpedLogs {
  /**
   * Arbitrary verbose data which will be printed on the app logger.
   */
  verbose?: any;

  /**
   * Small metadata dictionary which will be added directly to the report ticket.
   */
  metadata?: StringMap<string>;
}

let sequence = 0;
const observers: { [key: number]: LogMetadataProviderInfo } = {};

export type LogMetadataProvider = (includeMetadata?: boolean, includeVerbose?: boolean) => DumpedLogs;
export type Observer = () => void;

/**
 * Registers a callback which will be used to provide log metadata whenever the
 * user uses shake to report. The callback can return any js objects, arrays or
 * even a plain string. The result will be ultimately serialized in a JSON like
 * payload. The function returns an Observer which must be disposed when appropriate.
 *
 * @param name the name of the provider, which will be used to help identify where
 * the data is coming from.
 *
 * @param provider a callback which will be called when shake to report is used.
 */
export function registerLogMetadataProvider(name: string, provider: LogMetadataProvider): Observer {
  const key = ++sequence;
  observers[key] = { name, provider };

  return () => {
    delete observers[key];
  };
}

/**
 * Call all the registed providers and return an array containing the names and the provided data.
 */
export function dumpLogMetadata(includeMetadata: boolean, includeVerbose: boolean): any[] {
  const out: any[] = [];

  for (const key in observers) {
    const info = observers[key]!;

    let dumpedData: DumpedLogs;
    try {
      dumpedData = info.provider(includeMetadata, includeVerbose);
    } catch (err: any) {
      dumpedData = { verbose: `Provider threw an error while providing log data: ${err.message}:\n${err.stack}` };
    }

    out.push(info.name);
    out.push(includeVerbose ? dumpedData.verbose : undefined);
    out.push(includeMetadata ? dumpedData.metadata : undefined);
  }

  return out;
}
