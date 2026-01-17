import { Asset } from 'valdi_core/src/Asset';
import { makeCatalog } from 'valdi_core/src/AssetCatalog';
import 'jasmine/src/jasmine';

function makeAsset(path: string): Asset {
  return { path, width: 42, height: 42 };
}

describe('AssetCatalog', () => {
  it('builds properties from assets', () => {
    const file1 = makeAsset('module1:file1');
    const file2 = makeAsset('module1:file2');
    const catalog = makeCatalog([file1, file2]);

    expect(catalog.file1).toBe(file1);
    expect(catalog.file2).toBe(file2);
  });

  it('returns undefined on unknown assets', () => {
    const file1 = makeAsset('module1:file1');
    const file2 = makeAsset('module1:file2');
    const catalog = makeCatalog([file1, file2]);

    expect(catalog.file3).toBeUndefined();
  });

  it('convert snake_case into camelCase', () => {
    const file1 = makeAsset('nice:file_1');
    const fileBig = makeAsset('nice:file_big');
    const fileLarge = makeAsset('nice:file____large');

    const catalog = makeCatalog([file1, fileBig, fileLarge]);

    expect(catalog.file1).toBe(file1);
    expect(catalog.fileBig).toBe(fileBig);
    expect(catalog.fileLarge).toBe(fileLarge);
  });
});
