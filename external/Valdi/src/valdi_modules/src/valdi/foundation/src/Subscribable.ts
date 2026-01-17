import { Subscription } from './Subscription';

export class Subscribable<T> {
  private listeners: T[] = [];
  subscribe(listener: T): Subscription {
    this.listeners.push(listener);
    return {
      unsubscribe: () => {
        const index = this.listeners.indexOf(listener);
        if (index != -1) {
          this.listeners.splice(index, 1);
        }
      },
    };
  }
  subscribers(): T[] {
    return this.listeners;
  }
}
