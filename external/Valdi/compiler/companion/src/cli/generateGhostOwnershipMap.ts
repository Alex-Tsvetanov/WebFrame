import * as fs from 'fs/promises';
import * as path from 'path';

import * as fg from 'fast-glob';
import { ILogger } from '../logger/ILogger';

export type GenerateGhostOwnershipMapResult = { [key: string]: string };

export async function generateGhostOwnershipMap(
  logger: ILogger | undefined,
  outputDir: string,
): Promise<GenerateGhostOwnershipMapResult> {
  logger?.info?.('Generating ghost team attribution map...');
  const allModulesAndDepsPath = path.join(outputDir, 'android/ALL_MODULES_AND_DEPS.bzl');
  const outputPath = path.join(outputDir, 'android/ghost-team-attribution.json');
  let allModulesAndDepsContents = await fs.readFile(allModulesAndDepsPath, { encoding: 'utf-8' });
  allModulesAndDepsContents = allModulesAndDepsContents.slice('ALL_MODULES_AND_DEPS ='.length);
  const allModulesAndDeps = JSON.parse(allModulesAndDepsContents);

  const result: GenerateGhostOwnershipMapResult = {};

  for (let module in allModulesAndDeps) {
    const owner = allModulesAndDeps[module].owner;
    const moduledir = path.join(outputDir, `android/modules/${module}`);
    const debugdir = path.join(moduledir, `debug`);
    const srcdir = path.join(debugdir, 'src');
    const kotlinFiles = fg.sync([`${srcdir}/**/*.kt`], { dot: true });
    const classArtifactNames = kotlinFiles.map((file) => {
      // In: /Users/sahmed2/Snapchat/Dev/client/src/valdi_modules/generated-src/android/modules/context_cards/debug/src/com/snap/contextcards/valdi/model/ContextV2ErrorCardViewModel.kt
      // Out: com.snap.contextcards.valdi.model.ContextV2ErrorCardViewModel
      const withoutPrefix = file.slice(srcdir.length + 1);
      const withoutSuffix = withoutPrefix.slice(0, -'.kt'.length);
      const packageName = withoutSuffix.replace(/\//g, '.');
      return packageName;
    });

    const assets = fg.sync([`${moduledir}/*/assets/*`], { dot: true });
    const assetsArtifacts = assets.map((file) => {
      return `/assets/${path.basename(file)}`;
    });

    const idsXml = fg.sync([`${debugdir}/res/values/*_ids.xml`], { dot: true });
    const idsArtifacts = [];
    const idRegex = /<item type="id" name="(\w+)"/g;
    for (const idFile of idsXml) {
      const contents = await fs.readFile(idFile, { encoding: 'utf-8' });
      const matches = contents.matchAll(idRegex);
      for (const match of matches) {
        if (match[1]) {
          idsArtifacts.push(`R.id.${match[1]}`);
        }
      }
    }

    const stringsXml = fg.sync([`${moduledir}/*/res/values/valdi-strings*.xml`], { dot: true });
    const stringsArtifacts = [];
    const stringRegex = /<string name="(\w+)"/g;
    for (const stringsFile of stringsXml) {
      const contents = await fs.readFile(stringsFile, { encoding: 'utf-8' });
      const matches = contents.matchAll(stringRegex);
      for (const match of matches) {
        if (match[1]) {
          stringsArtifacts.push(`R.string.${match[1]}`);
        }
      }
    }

    const drawables = fg.sync([`${debugdir}/res/*/*`], { dot: true });
    const drawablesArtifactNames = drawables.flatMap((file) => {
      // In: /Users/sahmed2/Snapchat/Dev/client/src/valdi_modules/generated-src/android/modules/context_cards/debug/res/drawable-hdpi/context_cards_context_loading_placeholder.webp
      // Out: R.drawable.context_cards_context_loading_placeholder
      // Out (for every display density): /res/drawable-hdpi-v4/drawable.context_cards_context_loading_placeholder.webp
      const displayDensity = path.basename(path.dirname(file));
      const filename = path.basename(file);
      const filenameWithoutExtension = path.parse(filename).name;
      return [`/res/${displayDensity}-v4/${filename}`, `R.drawable.${filenameWithoutExtension}`];
    });
    const allArtifacts = [
      ...classArtifactNames,
      ...assetsArtifacts,
      ...idsArtifacts,
      ...stringsArtifacts,
      ...drawablesArtifactNames,
    ].sort((a, b) => {
      return a.localeCompare(b);
    });
    for (const artifact of allArtifacts) {
      result[artifact] = owner;
    }
  }

  fs.writeFile(outputPath, JSON.stringify(result));

  logger?.info?.(`Done! Count: ${Object.values(result).length}`);
  return result;
}
