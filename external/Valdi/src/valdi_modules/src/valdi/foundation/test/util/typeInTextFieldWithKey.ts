import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { wait } from 'valdi_core/src/utils/PromiseUtils';
import { IRenderedElementViewClass } from 'valdi_test/test/IRenderedElementViewClass';
import {
  EditTextBeginEvent,
  EditTextEndEvent,
  EditTextEvent,
  EditTextUnfocusReason,
} from 'valdi_tsx/src/NativeTemplateElements';
import { elementTypeFind } from './elementTypeFind';
import { findNodeWithKey } from './findNodeWithKey';
import { getAttributeFromNode } from './getAttributeFromNode';
import { isRenderedElement } from './isRenderedElement';
import { waitFor } from './waitFor';

/**
 * Execute an action after a short delay, to allow other synchronous actions to complete first.
 */
const waitAndRunNextAction = async (action: () => any): Promise<void> => {
  await wait(1);
  await action();
};

const rejectIfNotTypeable = (node: IRenderedElement[]): IRenderedElement => {
  if (!node.length) {
    throw new Error('textfield not found');
  }
  if (node.length > 1) {
    throw new Error('Multiple textfields found');
  }
  const textField = node[0];
  const enaled: boolean | undefined = getAttributeFromNode(textField, 'enaled');

  if (enaled === false) {
    throw new Error('textfield is disabled');
  }
  return textField;
};

const waitForTextFieldWithKey = async (
  node: IComponent | IRenderedElement,
  key: string,
  timeoutMs = 3000,
  intervalMs = 10,
): Promise<IComponent | IRenderedElement> => {
  try {
    return await waitFor(
      () => rejectIfNotTypeable(elementTypeFind(findNodeWithKey(node, key), IRenderedElementViewClass.TextField)),
      timeoutMs,
      intervalMs,
    );
  } catch (e) {
    throw new Error(
      `waitForTextFieldWithKey failed to type key "${key}" in node "${
        isRenderedElement(node) ? node.viewClass : node.renderer.contextId
      }" after ${timeoutMs}ms. ${e}`,
    );
  }
};

type TextEventFunc<T, R = void> = ((event: T) => R) | undefined;

/**
 * Type in a text field.
 * @param node The node or parent node contains the textfield to type in.
 * @param key The key of the textfield or parent node.
 * @param value The value to type in.
 * @param append Whether to append the value to the existing text.
 */
export const typeInTextFieldWithKey = async (
  node: IComponent | IRenderedElement,
  key: string,
  value: string,
  append: boolean = false,
): Promise<void> => {
  const textField = await waitForTextFieldWithKey(node, key);
  // We need to get the latest value before emitting each value, because consumers may have special formatting logic
  const getCurrentValue = () => getAttributeFromNode<string>(textField, 'value') ?? '';
  let currentValue = '';
  const onEditBegin: TextEventFunc<EditTextBeginEvent> = getAttributeFromNode(textField, 'onEditBegin');
  const onEditEnd: TextEventFunc<EditTextEndEvent> = getAttributeFromNode(textField, 'onEditEnd');
  const onChange: TextEventFunc<EditTextEvent> = getAttributeFromNode(textField, 'onChange');
  const onWillChange: TextEventFunc<EditTextEvent, EditTextEvent | undefined> = getAttributeFromNode(
    textField,
    'onWillChange',
  );

  // Start editing
  await waitAndRunNextAction(() => {
    currentValue = getCurrentValue();
    onEditBegin?.({
      text: currentValue,
      selectionStart: currentValue.length,
      selectionEnd: currentValue.length,
    });
  });

  // Clear the text field if not appending
  if (!append) {
    const event = {
      text: '',
      selectionStart: 0,
      selectionEnd: 0,
    };
    await waitAndRunNextAction(() => onWillChange?.(event));
    await waitAndRunNextAction(() => onChange?.(event));
  }

  // Type in the text field character by character
  for (const char of value.split('')) {
    currentValue = getCurrentValue();
    currentValue += char;
    const event = {
      text: currentValue,
      selectionStart: currentValue.length,
      selectionEnd: currentValue.length,
    };
    await waitAndRunNextAction(() => onWillChange?.(event));
    await waitAndRunNextAction(() => onChange?.(event));
  }

  // End editing
  await waitAndRunNextAction(() => {
    currentValue = getCurrentValue();
    onEditEnd?.({
      text: currentValue,
      selectionStart: currentValue.length,
      selectionEnd: currentValue.length,
      reason: EditTextUnfocusReason.DismissKeyPress,
    });
  });
};
