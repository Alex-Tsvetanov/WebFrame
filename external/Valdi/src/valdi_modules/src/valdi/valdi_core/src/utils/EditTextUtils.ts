import { EditTextEvent } from 'valdi_tsx/src/NativeTemplateElements';

export namespace EditTextUtils {
  /**
   * Create a new EditTextEvent with a modified text content
   * and make the best-effort to update to the selection
   */
  export function makeUpdatedEvent(from: EditTextEvent, newText: string): EditTextEvent {
    const originalText = from.text;
    const updatedText = newText;

    let updatedSelectionStart = from.selectionStart;
    let updatedSelectionEnd = from.selectionEnd;

    const diffChange = updatedText.length - originalText.length;

    // TODO - this is very simple diffing, should probably use a Meiyer algorithm here
    if (diffChange !== 0) {
      let firstChange = 0;
      for (let i = 0; i < originalText.length; i++) {
        if (originalText[i] !== updatedText[i]) {
          firstChange = i;
          break;
        }
      }

      if (firstChange <= updatedSelectionStart) {
        updatedSelectionStart += diffChange;
      }

      if (firstChange <= updatedSelectionEnd) {
        updatedSelectionEnd += diffChange;
      }
    }

    return {
      text: updatedText,
      selectionStart: updatedSelectionStart,
      selectionEnd: updatedSelectionEnd,
    };
  }
}
