import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { findNodeWithKey } from './findNodeWithKey';
import { getAttributeFromNode } from './getAttributeFromNode';
import { isRenderedElement } from './isRenderedElement';
import { waitFor } from './waitFor';

const DEFAULT_TIMEOUT_MS = 3000;
const DEFAULT_INTERVAL_MS = 10;

const rejectIfNotTappable = <T extends IComponent | IRenderedElement>(node: T | undefined): T => {
  if (!node) {
    throw new Error('Node not found');
  }
  const onTap = getAttributeFromNode(node, 'onTap');
  const disabled = getAttributeFromNode(node, 'disabled');
  const accessibilityStateDisabled = getAttributeFromNode(node, 'accessibilityStateDisabled');
  const onTapDisabled = getAttributeFromNode(node, 'onTapDisabled');
  const enabled = getAttributeFromNode(node, 'enabled') ?? true;
  if (disabled || !enabled || onTapDisabled || accessibilityStateDisabled) {
    throw new Error('Node is disabled');
  }
  if (!onTap) {
    throw new Error('onTap handler is not set');
  }
  return node;
};

const waitForNodeTappableWithKey = async (
  node: IComponent | IRenderedElement,
  key: string,
  timeoutMs = 3000,
  intervalMs = 10,
): Promise<IComponent | IRenderedElement> => {
  try {
    return await waitFor(() => rejectIfNotTappable(findNodeWithKey(node, key)[0]), timeoutMs, intervalMs);
  } catch (e) {
    throw new Error(
      `waitForNodeTappableWithKey failed to tap key "${key}" in node "${
        isRenderedElement(node) ? node.viewClass : node.renderer.contextId
      }" after ${timeoutMs}ms. ${e}`,
    );
  }
};

export const tapNodeWithKey = async (
  component: IComponent | IRenderedElement,
  key: string,
  timeoutMs = DEFAULT_TIMEOUT_MS,
  intervalMs = DEFAULT_INTERVAL_MS,
): Promise<void> => {
  const node = await waitForNodeTappableWithKey(component, key, timeoutMs, intervalMs);
  const onTap = getAttributeFromNode(node, 'onTap');
  await onTap?.();
};
