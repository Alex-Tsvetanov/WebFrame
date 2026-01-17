export interface ApplicationCancelable {
  cancel(): void;
}

export function observeEnteredBackground(observe: () => void): ApplicationCancelable;
export function observeEnteredForeground(observe: () => void): ApplicationCancelable;
export function observeKeyboardHeight(observe: (height: number) => void): ApplicationCancelable;
export function isForegrounded(): boolean;
export function isIntegrationTestEnvironment(): boolean;
export function getAppVersion(): string;
