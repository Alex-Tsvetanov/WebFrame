import { Component } from "valdi_core/src/Component";
import { registerLogMetadataProvider, DumpedLogs } from "valdi_core/src/BugReporter";

export class DumpLogTest extends Component {

  onCreate() {
    const observer = registerLogMetadataProvider('CustomLog', () => this.dumpCustomLog());
    this.registerDisposable(observer);
  }

  onRender() {
    <view>
      <layout/>
    </view>
  }

  private dumpCustomLog(): DumpedLogs {
    return {verbose: 'test123', metadata: {myKey: 'myValue'}};
  }

}