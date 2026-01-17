import fs from 'fs';
import os from 'os';
import path from 'path';
import yaml from 'yaml';
import * as Constants from '../core/constants';
import { CliError } from '../core/errors';

export type Replacements = { [key: string]: string };

export class TemplateFile {
  readonly templatePath: string;
  readonly metaTemplatePath: string;

  // Defaults to cwd
  readonly outputPath: string;
  readonly replacements: Replacements;

  // Name/Path without .template suffix
  readonly baseName: string;
  readonly basePath: string;

  private constructor(templatePath: string, outputPath?: string, replacements: Replacements = {}) {
    this.templatePath = path.normalize(templatePath);
    this.metaTemplatePath = path.join(Constants.META_DIR_PATH, this.templatePath);

    if (!fileExists(this.metaTemplatePath)) {
      throw new CliError(`Template file not found: ${this.metaTemplatePath}`);
    }

    const parsedPath = path.parse(this.templatePath);
    this.baseName = parsedPath.name;
    this.basePath = path.join(parsedPath.dir, parsedPath.name);

    this.outputPath = outputPath ?? path.join(process.cwd(), this.basePath);
    this.outputPath = path.normalize(this.outputPath);

    this.replacements = replacements;
  }

  static init(templatePath: string): TemplateFile {
    return new TemplateFile(templatePath);
  }

  withReplacements(replacements: Replacements): TemplateFile {
    return new TemplateFile(this.templatePath, this.outputPath, replacements);
  }

  withOutputPath(outputPath: string): TemplateFile {
    const newOutputPath = isDirectory(outputPath) ? path.join(outputPath, this.baseName) : outputPath;
    return new TemplateFile(this.templatePath, newOutputPath, this.replacements);
  }

  expandTemplate(): string {
    const metaTemplatePath = path.join(Constants.META_DIR_PATH, this.templatePath);
    return copyFileWithReplacement(metaTemplatePath, this.outputPath, this.replacements);
  }
}

export function resolveFilePath(pathString: string): string {
  if (pathString.startsWith('~')) {
    const tildeResolvedPath = path.join(os.homedir(), pathString.slice(1));
    return path.resolve(tildeResolvedPath);
  }
  return path.resolve(pathString);
}

export function fileExists(filePath: string): boolean {
  return fs.existsSync(filePath);
}

export function getUserConfig(): Constants.UserConfig {
  let resolvedPath: string | undefined;

  for (const filePath of Constants.VALDI_CONFIG_PATHS) {
    const unrolledPath = resolveFilePath(filePath);
    if (fs.existsSync(unrolledPath)) {
      resolvedPath = unrolledPath;
      break;
    }
  }

  if (resolvedPath === undefined) {
    throw new CliError(
      `Please create a Valdi config file in ${Constants.VALDI_CONFIG_PATHS[0] ?? ''} (Running 'valdi bootstrap' takes care of that for you)`,
    );
  }

  const configContents = fs.readFileSync(resolvedPath, 'utf8');
  const parsedYaml = yaml.parse(configContents) as Constants.UserConfig;

  return parsedYaml;
}

export function getAllFilePaths(directoryPath: string): string[] {
  directoryPath = resolveFilePath(directoryPath);

  const files: string[] = [];
  function traverseDirectories(dir: string) {
    fs.readdirSync(dir, { withFileTypes: true }).forEach(file => {
      const fullPath = path.join(dir, file.name);
      if (file.isDirectory()) {
        traverseDirectories(fullPath);
        return;
      }

      files.push(fullPath);
    });
  }

  traverseDirectories(directoryPath);

  return files;
}

export function isDirectoryEmpty(directoryPath: string): boolean {
  const absPath = resolveFilePath(directoryPath);
  const dir = fs.opendirSync(absPath);
  const firstEntry = dir.readSync();
  dir.closeSync();
  return firstEntry === null;
}

export function getFilesSortedByUpdatedTime(directoryPath: string): string[] {
  directoryPath = resolveFilePath(directoryPath);
  const files = getAllFilePaths(directoryPath);

  const fileStats = files.map(file => {
    const stats = fs.statSync(file);
    return { file, mtime: stats.mtime };
  });

  const sortedFiles = fileStats.sort((a, b) => b.mtime.getTime() - a.mtime.getTime()).map(fileStat => fileStat.file);

  return sortedFiles;
}

export function isDirectory(filePath: string): boolean {
  return fs.existsSync(filePath) && fs.statSync(filePath).isDirectory();
}

export function hasExtension(filePath: string, extension: string): boolean {
  return fs.existsSync(filePath) && path.extname(filePath) === extension;
}

export function processReplacements(input: string, replacements: Replacements): string {
  let outputContent = input;
  for (const [key, value] of Object.entries(replacements)) {
    const placeholder = `{{${key}}}`;
    const regex = new RegExp(placeholder, 'g'); // Global replace for all occurrences
    outputContent = outputContent.replace(regex, value);
  }

  return outputContent;
}

// Returns outputFilePath
export function copyFileWithReplacement(
  inputFilePath: string,
  outputFilePath: string,
  replacements: Replacements,
): string {
  // Read the input file content
  const inputContent = fs.readFileSync(inputFilePath, 'utf8');

  // Replace the keys in the input content with the corresponding values
  const outputContent = processReplacements(inputContent, replacements);

  // Ensure the output directory exists
  const outputDir = path.dirname(outputFilePath);
  if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
  }

  // Write the replaced content to the output file
  fs.writeFileSync(outputFilePath, outputContent);

  return outputFilePath;
}

export function deleteAll(destPath: string) {
  if (!isDirectory(destPath)) {
    return;
  }

  fs.readdirSync(destPath, { withFileTypes: true }).forEach(item => {
    const fullPath = path.join(destPath, item.name);
    fs.rmSync(fullPath, { recursive: true });
  });
}

// Util functions for copying files
export interface CopyConfig {
  whitelist?: string[];
  blacklist?: string[];
}

export function loadConfig(configPath: string): CopyConfig {
  const ext = path.extname(configPath).toLowerCase();
  const rawConfig = fs.readFileSync(configPath, 'utf8');

  if (ext === '.json') {
    return JSON.parse(rawConfig) as CopyConfig;
  } else {
    throw new Error(`Unsupported config file format: ${ext}`);
  }
}

function precompilePatterns(patterns?: string[]): RegExp[] {
  return patterns?.map(pattern => new RegExp(pattern)) ?? [];
}

function matchesCompiledPatterns(compiledPatterns: RegExp[], filePath: string): boolean {
  return compiledPatterns.some(regex => regex.test(filePath));
}

function isPathAllowed(filePath: string, blacklist: RegExp[], whitelist: RegExp[]): boolean {
  const normalizedPath = resolveFilePath(filePath);

  if (blacklist.length > 0 && matchesCompiledPatterns(blacklist, normalizedPath)) {
    return false;
  }

  if (whitelist.length > 0) {
    return matchesCompiledPatterns(whitelist, normalizedPath);
  }

  return true;
}

export function copyFiles(sourceDir: string, destDir: string, config: CopyConfig = {}) {
  // Precompile patterns for efficiency.
  const blacklist = precompilePatterns(config.blacklist);
  const whitelist = precompilePatterns(config.whitelist);

  fs.cpSync(sourceDir, destDir, {
    recursive: true,
    filter: (src): boolean => {
      return isPathAllowed(src, blacklist, whitelist);
    },
  });
}

export function removeFileOrDirAtPath(path: string): boolean {
  if (!fs.existsSync(path)) {
    return false;
  }

  const outputStat = fs.lstatSync(path);
  if (outputStat.isFile()) {
    fs.rmSync(path);
    return true;
  } else if (outputStat.isDirectory()) {
    fs.rmSync(path, { recursive: true });
    return true;
  }
  return false;
}
