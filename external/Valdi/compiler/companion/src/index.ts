import { CompilerCompanion } from './CompilerCompanion';

const companion = new CompilerCompanion(new Set(process.argv));
companion.start();
