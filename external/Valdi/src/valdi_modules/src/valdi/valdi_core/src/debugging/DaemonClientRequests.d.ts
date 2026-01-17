import { DaemonConnectedClient } from './DaemonClientManager';

export const enum DaemonClientRequestType {
  WRITE_FILE = 'write_file',
  READ_FILE = 'read_file',
  READ_DIRECTORY = 'read_directory',
  DELETE_FILE = 'delete_file',
}

interface DaemonClientRequestBase<Type extends DaemonClientRequestType, BodyType> {
  type: Type;
  body: BodyType;
}

export const enum DaemonClientReadDirectoryEntryType {
  UNKNOWN = 0,
  FILE = 1,
  DIRECTORY = 2,
}

export interface DaemonClientReadDirectoryEntry {
  type: DaemonClientReadDirectoryEntryType;
  filename: string;
  absolute_path: string;
}

export interface DaemonClientFileBody {
  path: string;
}

export interface DaemonClientWriteFileBody extends DaemonClientFileBody {
  data_base64?: string;
  data_string?: string;
}

export interface DaemonClientReadFileBody extends DaemonClientFileBody {
  output_format: 'base64' | 'string';
}

export interface DaemonClientWriteFileRequest
  extends DaemonClientRequestBase<DaemonClientRequestType.WRITE_FILE, DaemonClientWriteFileBody> {}
export interface DaemonClientWriteFileResponse {}

export interface DaemonClientReadFileRequest
  extends DaemonClientRequestBase<DaemonClientRequestType.READ_FILE, DaemonClientReadFileBody> {}
export interface DaemonClientReadFileResponse {
  data_base64?: string;
  data_string?: string;
}

export interface DaemonClientReadDirectoryRequest
  extends DaemonClientRequestBase<DaemonClientRequestType.READ_DIRECTORY, DaemonClientFileBody> {}
export interface DaemonClientReadDirectoryResponse {
  entries: DaemonClientReadDirectoryEntry[];
}

export interface DaemonClientDeleteFileRequest
  extends DaemonClientRequestBase<DaemonClientRequestType.DELETE_FILE, DaemonClientFileBody> {}
export interface DaemonClientDeleteFileResponse {}

export type DaemonClientFileRequest =
  | DaemonClientWriteFileRequest
  | DaemonClientReadFileRequest
  | DaemonClientReadDirectoryRequest
  | DaemonClientDeleteFileRequest;

export type DaemonClientFileResponse =
  | DaemonClientWriteFileResponse
  | DaemonClientReadFileResponse
  | DaemonClientReadDirectoryResponse
  | DaemonClientDeleteFileResponse;

export interface DaemonClientConfigureRequest {
  application_id: string;
  platform: string;
}

export interface DaemonListConnectedClientsRequest {}

export interface DaemonForwardClientPayloadRequest {
  client_id: number;
  payload_string: string;
}

export interface DaemonClientRequest {
  configure?: DaemonClientConfigureRequest;
  list_connected_clients?: DaemonListConnectedClientsRequest;
  forward_client_payload?: DaemonForwardClientPayloadRequest;
  file_manager?: DaemonClientFileRequest;
}

export interface DaemonClientConfigureResponse {}

export interface DaemonClientListConnectedClientsResponse {
  clients: DaemonConnectedClient[];
}

export interface DaemonClientForwardPayloadResponse {}

export interface DaemonServerError {
  error_message: string;
}

export interface DaemonServerResponse {
  configure?: DaemonClientConfigureResponse;
  list_connected_clients?: DaemonClientListConnectedClientsResponse;
  forward_client_payload?: DaemonClientForwardPayloadResponse;
  file_manager?: DaemonClientFileResponse;
  error?: DaemonServerError;
}
