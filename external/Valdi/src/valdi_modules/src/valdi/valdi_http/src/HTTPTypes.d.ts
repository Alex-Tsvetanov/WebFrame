import { StringMap } from 'coreutils/src/StringMap';

export const enum HTTPMethod {
  GET = 'GET',
  POST = 'POST',
  DELETE = 'DELETE',
  PUT = 'PUT',
  HEAD = 'HEAD',
}

export interface HTTPRequest {
  url: string;
  method: HTTPMethod;
  headers?: StringMap<string>;
  body?: ArrayBuffer | Uint8Array;
  priority?: number;
}

export interface HTTPResponse {
  headers: StringMap<string>;
  statusCode: number;
  body?: Uint8Array;
}
