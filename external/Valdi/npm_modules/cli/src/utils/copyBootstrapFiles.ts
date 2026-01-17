import fs from 'fs';
import path from 'path';
import { globSync } from 'glob';
import { ANSI_COLORS } from '../core/constants';
import type { Replacements } from './fileUtils';
import { copyFileWithReplacement, copyFiles, processReplacements } from './fileUtils';
import { wrapInColor } from './logUtils';

export function copyBootstrapFiles(from: string, to: string, replacements: Replacements) {
  copyFiles(from, to);

  const keys = Object.keys(replacements);

  const filenamesToReplace = globSync(keys.map(key => `**/{{${key}}}`));

  for (const filename of filenamesToReplace) {
    fs.renameSync(filename, processReplacements(filename, replacements));
  }

  const templateFiles = globSync(`${to}/**/*.template`);
  templateFiles.forEach(templateFile => {
    const parsedPath = path.parse(templateFile);
    console.log(
      wrapInColor(`Updating template file ${parsedPath.name} in ${parsedPath.dir}...`, ANSI_COLORS.YELLOW_COLOR),
    );
    const templateFileWithoutSuffix = templateFile.replace(/.template$/, '');
    fs.renameSync(templateFile, templateFileWithoutSuffix);
    copyFileWithReplacement(templateFileWithoutSuffix, templateFileWithoutSuffix, replacements);
  });
}
