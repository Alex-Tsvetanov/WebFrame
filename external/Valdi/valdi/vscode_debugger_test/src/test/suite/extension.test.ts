/// <reference path="../../../vscode.proposed.debugFocus.d.ts" />
import * as vscode from 'vscode';
import { describe, it, afterEach} from 'mocha';
import * as path from 'path';
import * as fs from 'fs';

class ValdiModuleDebugger {
	private workspace : vscode.WorkspaceFolder; 
	private session : vscode.DebugSession | undefined;

	constructor() {
		if (vscode.workspace.workspaceFolders === undefined) {
			throw new Error("Invalid workspace provided");
		}
		this.workspace = vscode.workspace.workspaceFolders[0];

		if (!fs.existsSync(path.resolve(this.getWorkspacePath(), "valdi_config.yaml"))) {
			throw new Error("Workspace URI does not match");
		}
	}

	async start() {
		const workSpaceConfig : any = vscode.workspace.getConfiguration('launch', this.workspace.uri).configurations[0]
		vscode.debug.startDebugging(this.workspace, workSpaceConfig as vscode.DebugConfiguration);
		this.session = await new Promise<vscode.DebugSession>(resolve =>
			vscode.debug.onDidStartDebugSession(s =>
			  '__pendingTargetId' in s.configuration ? resolve(s) : undefined,
			),
		);
	}

	async continue() {
		await vscode.commands.executeCommand("workbench.action.debug.continue");
		const frame = await new Promise<vscode.StackFrameFocus>(resolve => {
			vscode.debug.onDidChangeStackFrameFocus(s => s === undefined ||( s instanceof vscode.ThreadFocus)  ? undefined : resolve(s))
		});
	}

	async stop() {
		vscode.debug.stopDebugging(this.session);
		await new Promise<void>(resolve =>
			vscode.debug.onDidTerminateDebugSession(s => {
				if (s.id === this.session?.id) {
					resolve()
				}
			}),
		);	

	}

	async setBreakpoint(file : string, line : number, col: number = 0) {
		const fileURI = vscode.Uri.joinPath(this.workspace.uri, file);
		vscode.debug.addBreakpoints([
			new vscode.SourceBreakpoint(
			  new vscode.Location(fileURI, new vscode.Position(line, col)),),
		]);
	}

	async waitUntilBreakpoint() {
		const frame = await new Promise<vscode.StackFrameFocus>(resolve => {
			vscode.debug.onDidChangeStackFrameFocus(s => s === undefined ||( s instanceof vscode.ThreadFocus)  ? undefined : resolve(s))
		});
	}

	removeAllBreakpoint() {
		vscode.debug.removeBreakpoints(vscode.debug.breakpoints);
	}

	getWorkspacePath() {
		return this.workspace.uri.path;
	}
}

describe('Valdi Debugger Test', () => {	
	const valdiDebugger = new ValdiModuleDebugger();
	const debugFile = 'src/valdi/valdi_debugger_integration_test/src/DIApp.tsx';

	afterEach(async () => {
		await valdiDebugger.stop()
		valdiDebugger.removeAllBreakpoint()
	})

	it('Debugger Integration Breakpoint', async () => {
		await valdiDebugger.setBreakpoint(debugFile, 21);
		await valdiDebugger.start()
		await valdiDebugger.waitUntilBreakpoint();
		await valdiDebugger.continue();

	}).timeout(60000);
});
