export interface IRootComponentsManager {
  create(contextId: string, componentPath: string, viewModel: any, componentContext: any): void;
  render(contextId: string, viewModel: any): void;
  destroy(contextId: string): void;
  attributeChanged(contextId: string, nodeId: number, attributeName: string, attributeValue: any): void;
  callComponentFunction(contextId: string, functionName: string, parameters: any[]): any;
  onRuntimeIssue(contextId: string, isError: boolean, message: string): void;

  stashData(): any;
  restoreData(data: any): void;
}
