import { Component } from "valdi_core/src/Component";

interface ViewModel {
    shouldContinue: () => boolean
}

export class ANRDetection extends Component<ViewModel> {

    onCreate(): void {
        while (this.viewModel.shouldContinue()) {}
    }
}
