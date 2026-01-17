import * as path from 'path';

import { runTests } from '@vscode/test-electron';

async function main() {
	try {
		const basedir = path.resolve(__dirname, '../..');
		const workspacedir = path.resolve(basedir, '../../valdi_modules')
		
		await runTests({ 
			extensionDevelopmentPath : basedir, 
			extensionTestsPath :  path.resolve(__dirname, './suite/index'), 
			version : "insiders",
			reuseMachineInstall: true,
			launchArgs: [
				workspacedir,
				"--extensionTestsPath=${workspaceFolder}/out/test/suite/index",
			],
		});
	} catch (err) {
		console.error('Failed to run tests');
		process.exit(1);
	}
}

main();
