package @VALDI_APP_PACKAGE@

import com.snap.valdi.support.AppBootstrapper
import com.snap.valdi.support.AppBootstrapActivity

import com.snap.valdi.views.ValdiRootView

class StartActivity : AppBootstrapActivity() {
    override fun createRootView(bootstrapper: AppBootstrapper): ValdiRootView {
        return bootstrapper.setComponentPath("@VALDI_ROOT_COMPONENT_PATH@").createRootView()
    }
}
