import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { IRenderedElementViewClass } from 'valdi_test/test/IRenderedElementViewClass';
import { elementTypeFind } from './elementTypeFind';
import { findNodeWithKey } from './findNodeWithKey';
import { getAttributeFromNode } from './getAttributeFromNode';
import { isRenderedElement } from './isRenderedElement';
import { waitFor } from './waitFor';

type Node = IComponent | IRenderedElement;

const DEFAULT_TIMEOUT_MS = 3000;
const DEFAULT_INTERVAL_MS = 10;

const rejectIfEmpty = <T>(result: T): T => {
  if (!result || (Array.isArray(result) && !result.length)) {
    throw new Error('Component not found');
  }
  return result;
};

const rejectIfExist = <T>(result: T): T => {
  if (Array.isArray(result) ? result.length : result) {
    throw new Error('Component still exists');
  }
  return result;
};

export const waitForNodesWithKey = async (
  root: Node | (() => Node),
  key: string,
  timeoutMs = DEFAULT_TIMEOUT_MS,
  intervalMs = DEFAULT_INTERVAL_MS,
): Promise<Node[]> => {
  try {
    return await waitFor(() => rejectIfEmpty(findNodeWithKey(root, key)), timeoutMs, intervalMs);
  } catch (e) {
    const resolvedNode = typeof root === 'function' ? root() : root;
    throw new Error(
      `waitForNodesWithKey failed to find key "${key}" in root "${
        isRenderedElement(resolvedNode) ? resolvedNode.viewClass : resolvedNode.renderer.contextId
      }" after ${timeoutMs}ms. ${e}`,
    );
  }
};

export const waitForNodeWithKey = async (
  root: Node | (() => Node),
  key: string,
  timeoutMs = DEFAULT_TIMEOUT_MS,
  intervalMs = DEFAULT_INTERVAL_MS,
): Promise<Node> => {
  return (await waitForNodesWithKey(root, key, timeoutMs, intervalMs))[0];
};

export const waitForNodeWithKeyToDisappear = async (
  root: Node | (() => Node),
  key: string,
  timeoutMs = DEFAULT_TIMEOUT_MS,
  intervalMs = DEFAULT_INTERVAL_MS,
): Promise<void> => {
  try {
    await waitFor(() => rejectIfExist(findNodeWithKey(root, key)), timeoutMs, intervalMs);
  } catch (e) {
    const resolvedNode = typeof root === 'function' ? root() : root;
    throw new Error(
      `waitForNodeWithKeyToDisappear is still able to find key "${key}" in root "${
        isRenderedElement(resolvedNode) ? resolvedNode.viewClass : resolvedNode.renderer.contextId
      }" after ${timeoutMs}ms. ${e}`,
    );
  }
};

export const waitForNodeWithAttributeValue = async (
  root: Node | (() => Node),
  key: string,
  attributeKey: string,
  attributeValue: unknown,
  timeoutMs = DEFAULT_TIMEOUT_MS,
  intervalMs = DEFAULT_INTERVAL_MS,
): Promise<void> => {
  try {
    return await waitFor(
      () => {
        const node = rejectIfEmpty(findNodeWithKey(root, key))[0];
        const currentValue = getAttributeFromNode(node, attributeKey);
        if (currentValue !== attributeValue) {
          throw new Error(`Attribute ${attributeKey} is not set to ${attributeValue}. Current value: ${currentValue}.`);
        }
      },
      timeoutMs,
      intervalMs,
    );
  } catch (e) {
    const resolvedNode = typeof root === 'function' ? root() : root;
    throw new Error(
      `waitForNodeWithAttributeValue failed to wait for key "${key}" in root "${
        isRenderedElement(resolvedNode) ? resolvedNode.viewClass : resolvedNode.renderer.contextId
      }" after ${timeoutMs}ms. ${e}`,
    );
  }
};

export const waitForElementWithAttributeValue = async (
  elementClass: IRenderedElementViewClass,
  root: Node | (() => Node),
  key: string,
  attributeKey: string,
  attributeValue: unknown,
  timeoutMs = DEFAULT_TIMEOUT_MS,
  intervalMs = DEFAULT_INTERVAL_MS,
): Promise<void> => {
  try {
    return await waitFor(
      () => {
        const elements = elementTypeFind(findNodeWithKey(root, key), elementClass);
        if (!elements.length) {
          throw new Error(`Could not find any ${elementClass} under key ${key}`);
        }
        const currentValue = getAttributeFromNode(elements[0], attributeKey);
        if (currentValue !== attributeValue) {
          throw new Error(`Attribute ${attributeKey} is not set to ${attributeValue}. Current value: ${currentValue}.`);
        }
      },
      timeoutMs,
      intervalMs,
    );
  } catch (e) {
    const resolvedNode = typeof root === 'function' ? root() : root;
    throw new Error(
      `waitForElementWithAttributeValue failed to wait for key "${key}" in root "${
        isRenderedElement(resolvedNode) ? resolvedNode.viewClass : resolvedNode.renderer.contextId
      }" after ${timeoutMs}ms. ${e}`,
    );
  }
};

export const waitForComponentWithKey = async (
  root: Node | (() => Node),
  key: string,
  timeoutMs = DEFAULT_TIMEOUT_MS,
  intervalMs = DEFAULT_INTERVAL_MS,
): Promise<IComponent> => {
  const found = await waitForNodeWithKey(root, key, timeoutMs, intervalMs);
  if (isRenderedElement(found)) {
    throw new Error(`Expected component, but got rendered element: ${found.viewClass}`);
  }
  return found;
};
