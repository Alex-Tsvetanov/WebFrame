import { StatefulComponent } from '../Component';
import { getGlobalProviderSource, GlobalProviderSourceId } from './GlobalProviderSource';
import { IProviderSource } from './IProviderSource';
import { ProviderSourceComponent } from './ProviderComponent';

interface ViewModel {
  globalProviderSourceId: GlobalProviderSourceId;
}

interface State {
  slotKey: number;
  providerSource?: IProviderSource;
}

/**
 * WithGlobalProviderSource is a component that will expose a global ProviderSource to its
 * slot, making any dependencies included in the ProviderSource to be accessible within that subtree.
 */
export class WithGlobalProviderSource
  extends StatefulComponent<ViewModel, State>
  implements ProviderSourceComponent<ViewModel>
{
  state: State = {
    slotKey: 0,
  };

  onViewModelUpdate() {
    const newProviderSource = getGlobalProviderSource(this.viewModel.globalProviderSourceId);

    if (!newProviderSource) {
      throw new Error(`Could not resolve global ProviderSource with id ${this.viewModel.globalProviderSourceId}`);
    }

    if (newProviderSource !== this.state.providerSource) {
      this.setState({
        slotKey: this.state.slotKey + 1,
        providerSource: newProviderSource,
      });
    }
  }

  onRender(): void {
    <slot key={this.state.slotKey.toString()} />;
  }

  $getProviderSource(): IProviderSource | undefined {
    return this.state.providerSource;
  }
}
