import { jsx } from '../JSXBootstrap';
import { DaemonClientManager } from './DaemonClientManager';

export function getDaemonClientManager(): DaemonClientManager {
  return jsx.daemonClientManager;
}
