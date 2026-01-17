import { DaemonClientRequest, DaemonServerResponse } from './DaemonClientRequests';
import { DaemonClientMessage, DaemonClientMessageType, isAnyResponse, Messages } from './Messages';

export const enum DaemonClientEventType {
  CONNECTED = 1,
  DISCONNECTED = 2,
  RECEIVED_CLIENT_PAYLOAD = 3,
}

export const enum DaemonConnectedClientPlatformType {
  IOS = 0,
  ANDROID = 1,
}

export interface DaemonConnectedClient {
  client_id: number;
  platform: DaemonConnectedClientPlatformType;
  application_id?: string;
}

export interface IDaemonClient {
  connectionId: number;

  submitRequest(payload: DaemonClientRequest, callback: (result?: DaemonServerResponse) => void): void;
}

export class ReceivedDaemonClientMessage {
  constructor(readonly message: DaemonClientMessage, readonly client: IDaemonClient) {}

  respond(makeMessage: (requestId: string) => string) {
    this.client.submitRequest(
      {
        forward_client_payload: {
          client_id: this.message.senderClientId,
          payload_string: makeMessage(this.message.requestId),
        },
      },
      () => {},
    );
  }
}

export interface IDaemonClientManagerListener {
  onAvailabilityChanged?(available: boolean): void;

  onClientConnected?(client: IDaemonClient): void;
  onClientDisconnected?(client: IDaemonClient): void;
  onMessage?(message: ReceivedDaemonClientMessage): void;
}

const MESSAGE_TIMEOUT = 10000;

class PendingMessage {
  timeout?: number;

  constructor(
    readonly client: IDaemonClient,
    readonly requestId: string,
    private resolve?: (message: DaemonClientMessage) => void,
    private reject?: (error: any) => void,
  ) {}

  doResolve(message: DaemonClientMessage) {
    const resolve = this.resolve;
    if (resolve) {
      this.resolve = undefined;
      this.reject = undefined;
      clearTimeout(this.timeout);

      resolve(message);
    }
  }

  doReject(error: any) {
    const reject = this.reject;
    if (reject) {
      this.resolve = undefined;
      this.reject = undefined;
      clearTimeout(this.timeout);

      reject(error);
    }
  }
}

export class DaemonClientManager {
  readonly daemonClients: IDaemonClient[] = [];
  private listeners: IDaemonClientManagerListener[] = [];
  private idSequence = 0;
  private pendingMessages: PendingMessage[] = [];

  addListener(listener: IDaemonClientManagerListener) {
    this.listeners.push(listener);

    if (this.daemonClients.length) {
      if (listener.onAvailabilityChanged) {
        listener.onAvailabilityChanged(true);
      }

      if (listener.onClientConnected) {
        for (const daemonClient of this.daemonClients) {
          listener.onClientConnected(daemonClient);
        }
      }
    }
  }

  removeListener(listener: IDaemonClientManagerListener) {
    const index = this.listeners.indexOf(listener);
    if (index >= 0) {
      this.listeners.splice(index, 1);
    }
  }

  private fulfillMessage(requestId: string, onMessage: (message: PendingMessage) => void) {
    for (let i = 0; i < this.pendingMessages.length; i++) {
      const message = this.pendingMessages[i];
      if (message.requestId === requestId) {
        this.pendingMessages.splice(i, 1);
        onMessage(message);
      }
    }
  }

  submitRequest(daemonClient: IDaemonClient, request: DaemonClientRequest): Promise<DaemonServerResponse> {
    return new Promise((resolve, reject) => {
      daemonClient.submitRequest(request, data => {
        if (data?.error) {
          reject(data.error.error_message);
        } else if (!data) {
          reject(new Error('No data returned'));
        } else {
          try {
            resolve(data);
          } catch (err: any) {
            reject(err);
          }
        }
      });
    });
  }

  submitMessage(
    daemonClient: IDaemonClient,
    clientId: number,
    makeMessage: (requestId: string) => string,
  ): Promise<DaemonClientMessage> {
    return new Promise((resolve, reject) => {
      this.idSequence++;
      const requestId = this.idSequence.toString();

      const pendingMessage = new PendingMessage(daemonClient, requestId, resolve, reject);
      this.pendingMessages.push(pendingMessage);

      pendingMessage.timeout = setTimeout(() => {
        this.fulfillMessage(requestId, message => message.doReject('Request timed out'));
      }, MESSAGE_TIMEOUT);

      this.submitRequest(daemonClient, {
        forward_client_payload: { client_id: clientId, payload_string: makeMessage(requestId) },
      }).catch(error => {
        this.fulfillMessage(requestId, message => message.doReject(error));
      });
    });
  }

  onEvent(eventType: DaemonClientEventType, daemonClient: IDaemonClient, payload?: any) {
    switch (eventType) {
      case DaemonClientEventType.CONNECTED:
        if (this.daemonClients.indexOf(daemonClient) >= 0) {
          return;
        }

        this.daemonClients.push(daemonClient);

        for (const listener of this.listeners) {
          if (this.daemonClients.length === 1) {
            if (listener.onAvailabilityChanged) {
              listener.onAvailabilityChanged(true);
            }
          }

          if (listener.onClientConnected) {
            listener.onClientConnected(daemonClient);
          }
        }

        break;
      case DaemonClientEventType.DISCONNECTED: {
        const index = this.daemonClients.indexOf(daemonClient);
        if (index >= 0) {
          this.daemonClients.splice(index, 1);

          for (const listener of this.listeners) {
            if (!this.daemonClients.length) {
              if (listener.onAvailabilityChanged) {
                listener.onAvailabilityChanged(false);
              }
            }

            if (listener.onClientDisconnected) {
              listener.onClientDisconnected(daemonClient);
            }
          }
        }
        break;
      }
      case DaemonClientEventType.RECEIVED_CLIENT_PAYLOAD: {
        const message = Messages.parse(payload['senderClientId'], payload['payload']);

        if (message.type === DaemonClientMessageType.ERROR_RESPONSE) {
          this.fulfillMessage(message.requestId, pendingMessage => {
            pendingMessage.doReject(message.body);
          });
          return;
        }

        if (isAnyResponse(message)) {
          this.fulfillMessage(message.requestId, pendingMessage => {
            pendingMessage.doResolve(message);
          });
          return;
        }

        const receivedMessage = new ReceivedDaemonClientMessage(message, daemonClient);

        for (const listener of this.listeners) {
          if (listener.onMessage) {
            listener.onMessage(receivedMessage);
          }
        }
        break;
      }
    }
  }
}
