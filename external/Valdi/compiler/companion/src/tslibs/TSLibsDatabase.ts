export interface TSLibEntry {
  fileName: string;
  content: string;
}

export const DB_FILENAME = 'tslibs.json';

let sharedInstance: TSLibsDatabase | undefined;

export class TSLibsDatabase {
  constructor(readonly entries: TSLibEntry[]) {}

  static get(): TSLibsDatabase {
    if (!sharedInstance) {
      sharedInstance = new TSLibsDatabase(require('./tslibs.json') as TSLibEntry[]);
    }

    return sharedInstance;
  }
}
