package com.snap.valdi.views

import android.content.Context
import android.graphics.Rect
import android.os.Bundle
import android.view.MotionEvent
import android.view.KeyEvent
import android.view.View
import android.view.ViewParent
import android.view.accessibility.AccessibilityEvent
import android.view.accessibility.AccessibilityManager
import android.view.accessibility.AccessibilityRecord
import androidx.core.view.AccessibilityDelegateCompat
import androidx.core.view.ViewCompat
import androidx.core.view.ViewCompat.FocusDirection
import androidx.core.view.ViewCompat.FocusRealDirection
import androidx.core.view.accessibility.AccessibilityEventCompat
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat
import androidx.core.view.accessibility.AccessibilityNodeProviderCompat

/**
 * ValdiAccessibilityDelegate implements the android "AccessibilityDelegate" interface
 *
 * It is an abstract class that require sub-classing to manage the tree of virtual view nodes
 *
 * This class will handle the focus state of the accessibility, and delegate any fetching/listing to its subclass
 *
 * This class is heavily inspired by the ExploreByTouchHelper android class,
 * but with less safety-nets and more customization capabilities (and simplified logic)
 * This is useful because valdi handles complex cases (full recursive tree)
 * that might not be fully supported using the vanilla simple standard case (flat list of child)
 */
public abstract class ValdiAccessibilityDelegate(val host: View) : AccessibilityDelegateCompat() {

    private val nodeProvider: NodeProvider by lazy {
        NodeProvider(this)
    }

    private val context: Context
    private val manager: AccessibilityManager

    private var virtualViewIdFocusedAccessibility: Int = VIRTUAL_VIEW_ID_INVALID
    private var virtualViewIdFocusedKeyboard: Int = VIRTUAL_VIEW_ID_INVALID
    private var virtualViewIdHovered: Int = VIRTUAL_VIEW_ID_INVALID

    init {
        if (host == null) {
            throw IllegalArgumentException("Host may not be null")
        }
        context = host.getContext()
        manager = context.getSystemService(Context.ACCESSIBILITY_SERVICE) as AccessibilityManager
    }

    /**
     * Calls that will need to be subclassed
     */
    protected abstract fun getVirtualViewAt(x: Float, y: Float): Int
    protected abstract fun getVirtualViewNearby(virtualViewId: Int, direction: Int): Int

    protected abstract fun onPopulateEventForHost(event: AccessibilityEvent)
    protected abstract fun onPopulateNodeForHost(nodeInfo: AccessibilityNodeInfoCompat)
    protected abstract fun onPerformActionForHost(action: Int, arguments: Bundle?): Boolean

    protected abstract fun onPopulateEventForVirtualView(virtualViewId: Int, event: AccessibilityEvent)
    protected abstract fun onPopulateNodeForVirtualView(virtualViewId: Int, nodeInfo: AccessibilityNodeInfoCompat)
    protected abstract fun onPerformActionForVirtualView(virtualViewId: Int, action: Int, arguments: Bundle?): Boolean

    protected abstract fun onVirtualViewKeyboardFocusChanged(virtualViewId: Int, hasFocus: Boolean)

    /**
     * Overriding system calls, important entry points
     */
    override fun getAccessibilityNodeProvider(host: View): AccessibilityNodeProviderCompat {
        return nodeProvider
    }

    override fun onInitializeAccessibilityEvent(host: View, event: AccessibilityEvent) {
        super.onInitializeAccessibilityEvent(host, event)
        onPopulateEventForHost(event)
    }

    override fun onInitializeAccessibilityNodeInfo(host: View, nodeInfo: AccessibilityNodeInfoCompat) {
        super.onInitializeAccessibilityNodeInfo(host, nodeInfo)
        onPopulateNodeForHost(nodeInfo)
    }

    /**
     * Public important entry points
     */
    public fun dispatchHoverEvent(event: MotionEvent): Boolean {
        if (!manager.isEnabled() || !manager.isTouchExplorationEnabled()) {
            return false
        }
        return dispatchHoverEventRouting(event)
    }

    public fun dispatchKeyEvent(event: KeyEvent): Boolean {
        return dispatchKeyEventRouting(event)
    }

    public fun onFocusChanged(gainFocus: Boolean, direction: Int, previouslyFocusedRect: Rect?) {
        if (virtualViewIdFocusedKeyboard != VIRTUAL_VIEW_ID_INVALID) {
            clearKeyboardFocusForVirtualView(virtualViewIdFocusedKeyboard)
        }
    }

    /**
     * Handle hover events
     */
    private fun dispatchHoverEventRouting(event: MotionEvent): Boolean {
        return when (event.getAction()) {
            MotionEvent.ACTION_HOVER_MOVE -> dispatchHoverEventPress(event)
            MotionEvent.ACTION_HOVER_ENTER -> dispatchHoverEventPress(event)
            MotionEvent.ACTION_HOVER_EXIT -> dispatchHoverEventRelease(event)
            else -> false
        }
    }

    private fun dispatchHoverEventPress(event: MotionEvent): Boolean {
        val virtualViewId: Int = getVirtualViewAt(event.getX(), event.getY())
        dispatchHoverEventUpdateFocus(virtualViewId)
        return (virtualViewId != VIRTUAL_VIEW_ID_INVALID)
    }

    private fun dispatchHoverEventRelease(event: MotionEvent): Boolean {
        if (virtualViewIdHovered != VIRTUAL_VIEW_ID_INVALID) {
            dispatchHoverEventUpdateFocus(VIRTUAL_VIEW_ID_INVALID)
            return true
        }
        return false
    }

    private fun dispatchHoverEventUpdateFocus(virtualViewId: Int) {
        if (virtualViewIdHovered == virtualViewId) {
            return
        }
        val previousVirtualViewId: Int = virtualViewIdHovered
        virtualViewIdHovered = virtualViewId
        // Stay consistent with framework behavior by sending ENTER/EXIT pairs
        // in reverse order. This is accurate as of API 18.
        sendEventForVirtualView(virtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER)
        sendEventForVirtualView(previousVirtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT)
    }

    /**
     * Handle key events
     */
    private fun dispatchKeyEventRouting(event: KeyEvent): Boolean {
        val action = event.getAction()
        if (action != KeyEvent.ACTION_UP) {
            return when (event.getKeyCode()) {
                KeyEvent.KEYCODE_DPAD_LEFT -> dispatchKeyEventCaseDirection(event)
                KeyEvent.KEYCODE_DPAD_UP -> dispatchKeyEventCaseDirection(event)
                KeyEvent.KEYCODE_DPAD_RIGHT -> dispatchKeyEventCaseDirection(event)
                KeyEvent.KEYCODE_DPAD_DOWN -> dispatchKeyEventCaseDirection(event)
                KeyEvent.KEYCODE_DPAD_CENTER -> dispatchKeyEventCaseEnter(event)
                KeyEvent.KEYCODE_ENTER -> dispatchKeyEventCaseEnter(event)
                KeyEvent.KEYCODE_TAB -> dispatchKeyEventCaseTab(event)
                else -> false
            }
        }
        return false
    }

    private fun dispatchKeyEventCaseDirection(event: KeyEvent): Boolean {
        if (event.hasNoModifiers()) {
            val direction = when (event.getKeyCode()) {
                KeyEvent.KEYCODE_DPAD_LEFT -> View.FOCUS_LEFT
                KeyEvent.KEYCODE_DPAD_UP -> View.FOCUS_UP
                KeyEvent.KEYCODE_DPAD_RIGHT -> View.FOCUS_RIGHT
                else -> View.FOCUS_DOWN
            }
            return moveFocus(direction, null)
        }
        return false
    }

    private fun dispatchKeyEventCaseEnter(event: KeyEvent): Boolean {
        if (event.hasNoModifiers()) {
            if (event.getRepeatCount() == 0) {
                enterFocus()
                return true
            }
        }
        return false
    }

    private fun dispatchKeyEventCaseTab(event: KeyEvent): Boolean {
        if (event.hasNoModifiers()) {
            return moveFocus(View.FOCUS_FORWARD, null)
        } else if (event.hasModifiers(KeyEvent.META_SHIFT_ON)) {
            return moveFocus(View.FOCUS_BACKWARD, null)
        } else {
            return false
        }
    }

    /**
     * Manual control through keypresses of the focused (virtual) view
     */
    private fun enterFocus(): Boolean {
        if (virtualViewIdFocusedKeyboard != VIRTUAL_VIEW_ID_INVALID) {
            val action = AccessibilityNodeInfoCompat.ACTION_CLICK
            return performAction(virtualViewIdFocusedKeyboard, action, null)
        }
        return false
    }

    private fun moveFocus(direction: Int, previouslyFocusedRect: Rect?): Boolean {
        if (virtualViewIdFocusedKeyboard != VIRTUAL_VIEW_ID_INVALID) {
            val virtualViewId = getVirtualViewNearby(virtualViewIdFocusedKeyboard, direction)
            if (virtualViewId != VIRTUAL_VIEW_ID_INVALID) {
                val action = AccessibilityNodeInfoCompat.ACTION_FOCUS
                return performAction(virtualViewId, action, null)
            }
        }
        return false
    }

    /**
     * Invalidation (public calls)
     */
    public fun invalidateRoot() {
        invalidateVirtualView(VIRTUAL_VIEW_ID_HOST, AccessibilityEventCompat.CONTENT_CHANGE_TYPE_SUBTREE)
    }

    public fun invalidateVirtualView(virtualViewId: Int) {
        invalidateVirtualView(virtualViewId, AccessibilityEventCompat.CONTENT_CHANGE_TYPE_UNDEFINED)
    }

    public fun invalidateVirtualView(virtualViewId: Int, changeTypes: Int) {
        if (virtualViewId != VIRTUAL_VIEW_ID_INVALID && manager.isEnabled()) {
            val parent = host.getParent()
            if (parent != null) {
                val event = createEvent(virtualViewId, AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED)
                AccessibilityEventCompat.setContentChangeTypes(event, changeTypes)
                parent.requestSendAccessibilityEvent(host, event)
            }
        }
    }

    /**
     * Creating accessibility events
     */
    private fun createEvent(virtualViewId: Int, eventType: Int): AccessibilityEvent {
        return when (virtualViewId) {
            VIRTUAL_VIEW_ID_HOST -> createEventForHost(eventType)
            else -> createEventForChild(virtualViewId, eventType)
        }
    }

    private fun createEventForHost(eventType: Int): AccessibilityEvent {
        val event = createEventInstance(eventType, null)
        host.onInitializeAccessibilityEvent(event)
        return event
    }

    private fun createEventForChild(virtualViewId: Int, eventType: Int): AccessibilityEvent {
        val event = createEventInstance(eventType, virtualViewId)
        onPopulateEventForVirtualView(virtualViewId, event)
        return event
    }

    private fun createEventInstance(eventType: Int, virtualViewId: Int?): AccessibilityEvent {
        val event = AccessibilityEvent.obtain(eventType)
        event.setEnabled(true)
        event.setPackageName(host.getContext().getPackageName())
        if (virtualViewId != null) {
            event.setSource(host, virtualViewId)
        } else {
            event.setSource(host)
        }
        return event
    }

    /**
     * Creating accessibility node infos
     */
    private fun createNode(virtualViewId: Int): AccessibilityNodeInfoCompat {
        return when (virtualViewId) {
            VIRTUAL_VIEW_ID_HOST -> createNodeForHost()
            else -> createNodeForChild(virtualViewId)
        }
    }

    private fun createNodeForHost(): AccessibilityNodeInfoCompat {
        val node = createNodeInstance(null)
        ViewCompat.onInitializeAccessibilityNodeInfo(host, node)
        return node
    }

    private fun createNodeForChild(virtualViewId: Int): AccessibilityNodeInfoCompat {
        val node = createNodeInstance(virtualViewId)
        if (virtualViewIdFocusedAccessibility == virtualViewId) {
            node.setAccessibilityFocused(true)
            node.addAction(AccessibilityNodeInfoCompat.ACTION_CLEAR_ACCESSIBILITY_FOCUS)
        } else if (node.isFocusable()) {
            node.setAccessibilityFocused(false)
            node.addAction(AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS)
        }
        if (virtualViewIdFocusedKeyboard == virtualViewId) {
            node.setFocused(true)
            node.addAction(AccessibilityNodeInfoCompat.ACTION_CLEAR_FOCUS)
        } else if (node.isFocusable()) {
            node.setFocused(false)
            node.addAction(AccessibilityNodeInfoCompat.ACTION_FOCUS)
        }
        onPopulateNodeForVirtualView(virtualViewId, node)
        return node
    }

    private fun createNodeInstance(virtualViewId: Int?): AccessibilityNodeInfoCompat {
        val node = AccessibilityNodeInfoCompat.obtain()
        node.setFocusable(true)
        node.setEnabled(true)
        node.setVisibleToUser(true)
        node.setPackageName(host.getContext().getPackageName())
        if (virtualViewId != null) {
            node.setSource(host, virtualViewId)
        } else {
            node.setSource(host)
        }
        return node
    }

    /**
     * Perform action routing
     */
    private fun performAction(virtualViewId: Int, action: Int, arguments: Bundle?): Boolean {
        return when (virtualViewId) {
            VIRTUAL_VIEW_ID_HOST -> performActionForHost(action, arguments)
            else -> performActionForChild(virtualViewId, action, arguments)
        }
    }

    private fun performActionForHost(action: Int, arguments: Bundle?): Boolean {
        val handledNative = ViewCompat.performAccessibilityAction(host, action, arguments)
        val handledCustom = onPerformActionForHost(action, arguments)
        return handledNative || handledCustom
    }

    private fun performActionForChild(virtualViewId: Int, action: Int, arguments: Bundle?): Boolean {
        val handledNative = when (action) {
            AccessibilityNodeInfoCompat.ACTION_FOCUS -> requestKeyboardFocusForVirtualView(virtualViewId)
            AccessibilityNodeInfoCompat.ACTION_CLEAR_FOCUS -> clearKeyboardFocusForVirtualView(virtualViewId)
            AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS -> requestAccessibilityFocus(virtualViewId)
            AccessibilityNodeInfoCompat.ACTION_CLEAR_ACCESSIBILITY_FOCUS -> clearAccessibilityFocus(virtualViewId)
            else -> false
        }
        val handledCustom = onPerformActionForVirtualView(virtualViewId, action, arguments)
        return handledNative || handledCustom
    }

    /**
     * Regular "Keyboard" selection focus control
     */
    private fun requestKeyboardFocusForVirtualView(virtualViewId: Int): Boolean {
        if (!host.isFocused() && !host.requestFocus()) {
            return false
        }
        if (virtualViewIdFocusedKeyboard == virtualViewId) {
            return false
        }
        if (virtualViewIdFocusedKeyboard != VIRTUAL_VIEW_ID_INVALID) {
            clearKeyboardFocusForVirtualView(virtualViewIdFocusedKeyboard)
        }
        if (virtualViewId == VIRTUAL_VIEW_ID_INVALID) {
            return false
        }
        virtualViewIdFocusedKeyboard = virtualViewId
        onVirtualViewKeyboardFocusChanged(virtualViewId, true)
        sendEventForVirtualView(virtualViewId, AccessibilityEvent.TYPE_VIEW_FOCUSED)
        return true
    }

    private fun clearKeyboardFocusForVirtualView(virtualViewId: Int): Boolean {
        if (virtualViewIdFocusedKeyboard != virtualViewId) {
            return false
        }
        virtualViewIdFocusedKeyboard = VIRTUAL_VIEW_ID_INVALID
        onVirtualViewKeyboardFocusChanged(virtualViewId, false)
        sendEventForVirtualView(virtualViewId, AccessibilityEvent.TYPE_VIEW_FOCUSED)
        return true
    }

    /**
     * Accessibility-specific focus control
     */
    private fun requestAccessibilityFocus(virtualViewId: Int):Boolean {
        if (!manager.isEnabled() || !manager.isTouchExplorationEnabled()) {
            return false
        }
        if (virtualViewIdFocusedAccessibility != virtualViewId) {
            if (virtualViewIdFocusedAccessibility != VIRTUAL_VIEW_ID_INVALID) {
                sendEventForVirtualView(virtualViewIdFocusedAccessibility, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED)
            }
            virtualViewIdFocusedAccessibility = virtualViewId
            host.invalidate()
            sendEventForVirtualView(virtualViewId, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED)
            return true
        }
        return false
    }

    private fun clearAccessibilityFocus(virtualViewId: Int): Boolean {
        if (virtualViewIdFocusedAccessibility == virtualViewId) {
            virtualViewIdFocusedAccessibility = VIRTUAL_VIEW_ID_INVALID
            host.invalidate()
            sendEventForVirtualView(virtualViewId, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED)
            return true
        }
        return false
    }

    /**
     * Util to notify the parent hierarchy of an event
     */
    private fun sendEventForVirtualView(virtualViewId: Int, eventType: Int): Boolean {
        if ((virtualViewId == VIRTUAL_VIEW_ID_INVALID) || !manager.isEnabled()) {
            return false
        }
        val parent = host.getParent()
        if (parent == null) {
            return false
        }
        val event = createEvent(virtualViewId, eventType)
        return parent.requestSendAccessibilityEvent(host, event)
    }

    /**
     * NodeProvider implementation (just redirects to the parent instance)
     */
    private class NodeProvider(val delegate: ValdiAccessibilityDelegate) : AccessibilityNodeProviderCompat() {
        override fun createAccessibilityNodeInfo(virtualViewId: Int): AccessibilityNodeInfoCompat {
            return delegate.createNode(virtualViewId)
        }
        override fun performAction(virtualViewId: Int, action: Int, arguments: Bundle?): Boolean {
            return delegate.performAction(virtualViewId, action, arguments)
        }
        override fun findFocus(focusType: Int): AccessibilityNodeInfoCompat? {
            val focusedId = if (focusType == AccessibilityNodeInfoCompat.FOCUS_ACCESSIBILITY) {
                delegate.virtualViewIdFocusedAccessibility
            } else {
                delegate.virtualViewIdFocusedKeyboard
            }
            if (focusedId == VIRTUAL_VIEW_ID_INVALID) {
                return null
            }
            return createAccessibilityNodeInfo(focusedId)
        }
    }

    /**
     * Global useful values
     */
    companion object {
        val VIRTUAL_VIEW_ID_INVALID = Integer.MIN_VALUE
        val VIRTUAL_VIEW_ID_HOST = View.NO_ID
    }

}
