import { ComponentConstructor, IComponent } from './IComponent';
import { DaemonClientManager } from './debugging/DaemonClientManager';

export interface EntryPointViewModel {
  rootConstructor: ComponentConstructor<IComponent>;
  rootViewModel: any;
  rootContext: any;
  daemonClientManager?: DaemonClientManager;
  enableErrorBoundary: boolean;
}
