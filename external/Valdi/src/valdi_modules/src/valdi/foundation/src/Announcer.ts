import { ICancelable } from 'foundation/src/ICancelable';
import CallbackCancelable from 'foundation/src/impl/CallbackCancelable';

export type Listener<T> = (value: T) => void;

interface ListenerInstance<T> {
  readonly listener: Listener<T>;
}

export class Announcer<T = void> {
  private readonly listenerInstances = new Set<ListenerInstance<T>>();

  announce(value: T): void {
    for (const instance of Array.from(this.listenerInstances)) {
      instance.listener(value);
    }
  }

  addListener(listener: Listener<T>): ICancelable {
    const instance: ListenerInstance<T> = { listener };
    this.listenerInstances.add(instance);
    return new CallbackCancelable(() => {
      this.listenerInstances.delete(instance);
    });
  }

  clear() {
    this.listenerInstances.clear();
  }
}
