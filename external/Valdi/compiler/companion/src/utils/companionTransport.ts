import { ResponseBody, TransportPayload } from '../protocol';

export function send(payload: TransportPayload) {
  const output = JSON.stringify(payload) + '\n';
  process.stdout.write(output);
}

/**
 * Send a successful response to a Compiler request with ID `id`
 */

export function sendResponse(id: string, body: ResponseBody) {
  send({
    id: id,
    body,
  });
}

/**
 * Send a failed response to a Compiler request with ID `id`
 */
export function sendError(id: string, err: Error) {
  send({
    id: id,
    error: `CompilerCompanion error: (rid: ${id}) ${err.message}`,
  });
}

/**
 * Notify the Compiler that an event has occurred.
 *
 * Events are sent without an `id`, since they can happen at any time,
 * without necessarily corresponding to processing a particular request
 */
export function sendEvent(type: string, message: string) {
  send({ event: { type, message: message } });
}
