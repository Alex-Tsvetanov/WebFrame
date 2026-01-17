package com.snap.valdi.support

import com.snap.valdi.makeScoped
import com.snap.valdi.ValdiRuntimeManager
import com.snap.valdi.IValdiRuntime
import com.snap.valdi.navigation.INavigator
import com.snap.valdi.views.ValdiRootView
import com.snap.valdi.snapdrawing.SnapDrawingOptions
import com.snap.valdi.snapdrawing.SnapDrawingRenderMode
import com.snap.valdi.RenderBackend
import com.snap.valdi.ValdiContextConfiguration

/**
  AppBootstrapper helps with bootstrapping a Valdi application.
  Users should call setComponentPath() to specify the root Valdi component
  path to use for application.
 */
class AppBootstrapper(val runtimeManager: ValdiRuntimeManager, val runtime: IValdiRuntime, val navigator: INavigator) {

    private var useSnapDrawing = false
    private var componentPath: String? = null

    /**
      Sets that the Valdi application should use the SnapDrawing render backend
      instead of the Android render backend. SnapDrawing is a more efficient
      render backend but it has been less tested.
     */
    fun setUseSnapDrawingRenderBackend(useSnapDrawing: Boolean): AppBootstrapper {
        this.useSnapDrawing = useSnapDrawing
        return this
    }

    /**
      Specify the component path to use for the application. The component
      path is in the form of '<ClassName>@<<path_to_file>', for example
      'MyComponent@my_module/src/File' would refer to the MyComponent exported
      class in my_module/src/File on the TypeScript side.
     */
    fun setComponentPath(componentPath: String): AppBootstrapper {
        this.componentPath = componentPath
        return this
    }

    /**
      Create the root view that will be used for the application, which will be
      configured appropriately depending on the options that were set on the
      AppBootstrapper.
     */
    fun createRootView(): ValdiRootView {
        val componentPath = this.componentPath ?: throw Exception("No componentPath specified")

        val renderBackend = if (useSnapDrawing) RenderBackend.SNAP_DRAWING else RenderBackend.ANDROID
        val snapDrawingOptions = SnapDrawingOptions(enableSynchronousDraw = true,
                                                    renderMode = SnapDrawingRenderMode.SURFACE_VIEW)
        val configuration = ValdiContextConfiguration(renderBackend, snapDrawingOptions)
        val scopedRuntime = runtime.makeScoped(configuration)

        val rootView = ValdiRootView(scopedRuntime.context)
        rootView.setRetainsLayoutSpecsOnInvalidateLayout(true)

        scopedRuntime.inflateViewAsync(rootView, componentPath, null, null, null, null)

        return rootView
    }

}