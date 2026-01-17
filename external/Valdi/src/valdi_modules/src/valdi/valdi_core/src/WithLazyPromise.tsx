import { StatefulComponent } from 'valdi_core/src/Component';
import { toError } from 'valdi_core/src/utils/ErrorUtils';
import { ILazyPromise } from 'valdi_core/src/utils/LazyPromise';
import { ComponentConstructor, IComponent } from './IComponent';

/**
 * Returns a new component constructor that will render the given component constructor
 * provided as a LazyPromise.
 */
export function withLazyPromise<T extends IComponent>(
  lazyPromise: ILazyPromise<ComponentConstructor<T>>,
): ComponentConstructor<IComponent<T['viewModel'], T['context']>, T['viewModel'], T['context']> {
  interface State {
    ctor?: ComponentConstructor<T>;
  }

  return class LazyPromiseComponent extends StatefulComponent<T['viewModel'], State, T['context']> {
    state: State = {};

    onCreate(): void {
      if (lazyPromise.value === undefined) {
        lazyPromise.promise
          .then(value => {
            this.onPromiseResolved(value);
          })
          .catch(error => {
            this.onPromiseFailed(error);
          });
      } else {
        this.setState({
          ctor: lazyPromise.value,
        });
      }
    }

    onRender(): void {
      const Ctor = this.state.ctor;
      if (Ctor) {
        <Ctor {...this.viewModel} children={(this.viewModel as any).children} />;
      }
    }

    private onPromiseResolved(ctor: ComponentConstructor<T>): void {
      if (this.isDestroyed()) {
        return;
      }

      this.setState({ ctor });
    }

    private onPromiseFailed(error: unknown): void {
      if (this.isDestroyed()) {
        return;
      }

      this.renderer.onUncaughtError('LazyPromise failed to resolve', toError(error), this);
    }
  };
}
