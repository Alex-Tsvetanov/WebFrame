export interface IWorkerService<T> {
  readonly api: T;
  dispose(): void;
}

export interface IWorkerServiceClient<T> {
  readonly serviceId: number;
  readonly api: T;
  dispose(): void;
}
