import { IComponent } from './IComponent';

export interface IEntryPointComponent extends IComponent {
  rootComponent: IComponent | undefined;
  forwardCall(callName: string, parameters: any[]): any;
}
