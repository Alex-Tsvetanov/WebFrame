export interface IComponentBase<ViewModel = unknown, Context = unknown> {
  viewModel: ViewModel;
  readonly context?: Context;

  onCreate(): void;
  onRender(): void;
  onDestroy(): void;
  onViewModelUpdate(oldViewModel: ViewModel): void;

  /**
   * Called whenever there was a rendering error in one of the child components
   * @param error
   */
  onError?(error: Error): void;
}
