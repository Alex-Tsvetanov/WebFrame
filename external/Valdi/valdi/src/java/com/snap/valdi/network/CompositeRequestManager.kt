package com.snap.valdi.network

import com.snapchat.client.valdi_core.Cancelable
import com.snapchat.client.valdi_core.HTTPRequest
import com.snapchat.client.valdi_core.HTTPRequestManager
import com.snapchat.client.valdi_core.HTTPRequestManagerCompletion
import com.snapchat.client.valdi_core.HTTPResponse

class CompositeRequestManager(): HTTPRequestManager() {

  private data class ChildRequestManager(
    val scheme: String,
    val requestManager: HTTPRequestManager
  ) {}

  private val childRequestManagers = mutableListOf<ChildRequestManager>()

  fun addRequestManager(scheme: String, requestManager: HTTPRequestManager) {
    childRequestManagers.add(ChildRequestManager(scheme + "://", requestManager))
  }

  override fun performRequest(request: HTTPRequest, completion: HTTPRequestManagerCompletion): Cancelable {
    val url = request.url
    for (childRequestManager in this.childRequestManagers) {
      if (url.startsWith(childRequestManager.scheme)) {
        return childRequestManager.requestManager.performRequest(request, completion)
      }
    }
    completion.onFail("No known protocol registered for url:" + url?.toString())
    return object: Cancelable() {
      override fun cancel() {}
    }
  }

}
