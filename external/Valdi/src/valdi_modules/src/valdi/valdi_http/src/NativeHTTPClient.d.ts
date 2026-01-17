import { HTTPRequest, HTTPResponse } from './HTTPTypes';

export function performRequest(
  request: HTTPRequest,
  completion: (response: HTTPResponse | undefined, error: unknown | undefined) => void,
): () => void;
