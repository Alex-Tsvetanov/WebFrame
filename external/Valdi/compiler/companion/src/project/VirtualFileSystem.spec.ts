import 'ts-jest';
import { VirtualFileSystem } from './VirtualFileSystem';

describe('VirtualFileSystem', () => {
  it('can insert directories', () => {
    const fs = new VirtualFileSystem<string>();

    expect(fs.directoryExists(['hello'])).toBeFalsy();
    expect(fs.directoryExists(['hello', 'world'])).toBeFalsy();
    expect(fs.directoryExists(['hello', 'world', 'nested'])).toBeFalsy();

    fs.addDirectory(['hello', 'world']);

    expect(fs.directoryExists(['hello'])).toBeTruthy();
    expect(fs.directoryExists(['hello', 'world'])).toBeTruthy();
    expect(fs.directoryExists(['hello', 'world', 'nested'])).toBeFalsy();
  });

  it('can insert files', () => {
    const fs = new VirtualFileSystem<string>();

    expect(fs.fileExists(['hello'])).toBeFalsy();
    expect(fs.fileExists(['hello', 'world'])).toBeFalsy();
    expect(fs.fileExists(['hello', 'world', 'nested'])).toBeFalsy();

    expect(fs.directoryExists(['hello'])).toBeFalsy();
    expect(fs.directoryExists(['hello', 'world'])).toBeFalsy();
    expect(fs.directoryExists(['hello', 'world', 'nested'])).toBeFalsy();

    fs.addFile(['hello', 'world', 'nested'], 'Nice!');

    expect(fs.fileExists(['hello'])).toBeFalsy();
    expect(fs.fileExists(['hello', 'world'])).toBeFalsy();
    expect(fs.fileExists(['hello', 'world', 'nested'])).toBeTruthy();

    expect(fs.directoryExists(['hello'])).toBeTruthy();
    expect(fs.directoryExists(['hello', 'world'])).toBeTruthy();
    expect(fs.directoryExists(['hello', 'world', 'nested'])).toBeFalsy();

    expect(fs.getFile(['hello', 'world', 'nested'])).toEqual('Nice!');
  });

  it('can rm file', () => {
    const fs = new VirtualFileSystem<string>();

    fs.addFile(['hello', 'world', 'nested'], 'Nice!');

    expect(fs.fileExists(['hello', 'world', 'nested'])).toBeTruthy();

    fs.remove(['hello', 'world', 'nested']);

    expect(fs.fileExists(['hello', 'world', 'nested'])).toBeFalsy();
    expect(fs.getFile(['hello', 'world', 'nested'])).toBeUndefined();

    expect(fs.directoryExists(['hello'])).toBeTruthy();
    expect(fs.directoryExists(['hello', 'world'])).toBeTruthy();
    expect(fs.directoryExists(['hello', 'world', 'nested'])).toBeFalsy();
  });

  it('can rm directory', () => {
    const fs = new VirtualFileSystem<string>();

    fs.addDirectory(['hello', 'world', 'nested']);

    expect(fs.directoryExists(['hello', 'world', 'nested'])).toBeTruthy();

    expect(fs.remove(['hello', 'world'])).toBeTruthy();

    expect(fs.directoryExists(['hello'])).toBeTruthy();
    expect(fs.directoryExists(['hello', 'world'])).toBeFalsy();
    expect(fs.directoryExists(['hello', 'world', 'nested'])).toBeFalsy();
  });

  it('can get directory entry names', () => {
    const fs = new VirtualFileSystem<string>();

    fs.addDirectory(['hello', 'world', 'nested']);
    fs.addFile(['hello', 'world', 'nested', 'cool'], 'Nice!');
    fs.addFile(['hello', 'world', 'a file'], 'Great');

    expect(fs.getDirectoryEntryNames([])).toEqual(['hello']);
    expect(fs.getDirectoryEntryNames(['hello'])).toEqual(['world']);
    expect(fs.getDirectoryEntryNames(['hello', 'world'])).toEqual(['nested', 'a file']);
    expect(fs.getDirectoryEntryNames(['hello', 'world', 'nested'])).toEqual(['cool']);
  });

  it('returns directory entry names when adding file', () => {
    const fs = new VirtualFileSystem<string>();

    fs.addFile(['hello', 'world', 'a file'], 'Great');

    expect(fs.getDirectoryEntryNames([])).toEqual(['hello']);
    expect(fs.getDirectoryEntryNames(['hello'])).toEqual(['world']);
    expect(fs.getDirectoryEntryNames(['hello', 'world'])).toEqual(['a file']);
  });

  it('throws when inserting directory with a file', () => {
    const fs = new VirtualFileSystem<string>();

    fs.addFile(['hello', 'world', 'nested'], 'Nice!');

    expect(fs.addDirectory(['hello'])).toBeFalsy();

    expect(fs.addDirectory(['hello', 'world'])).toBeFalsy();

    expect(() => {
      fs.addDirectory(['hello', 'world', 'nested']);
    }).toThrowError('Cannot create directory "nested": "/hello/world/nested" is a file');

    expect(() => {
      fs.addDirectory(['hello', 'world', 'nested', 'more']);
    }).toThrowError('Cannot create directory "nested": "/hello/world/nested" is a file');
  });

  it('throws when inserting file to replace directory', () => {
    const fs = new VirtualFileSystem<string>();

    fs.addDirectory(['hello', 'world', 'nested']);

    expect(() => {
      fs.addFile(['hello'], 'Nope');
    }).toThrowError('Cannot create file "hello": "/hello" is a directory');

    expect(() => {
      fs.addFile(['hello', 'world'], 'Nope');
    }).toThrowError('Cannot create file "world": "/hello/world" is a directory');

    expect(() => {
      fs.addFile(['hello', 'world', 'nested'], 'Nope');
    }).toThrowError('Cannot create file "nested": "/hello/world/nested" is a directory');

    fs.addFile(['hello', 'world', 'nested', 'more'], 'Nope');
  });
});
