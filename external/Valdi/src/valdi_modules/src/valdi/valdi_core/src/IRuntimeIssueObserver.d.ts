export interface IRuntimeIssueObserver {
  onRuntimeIssue(isError: boolean, message: string): void;
}
