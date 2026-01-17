export interface CustomMessageHandler {
  /**
   * Called whenever a custom message was received.
   * The function should be return a promise with the response if
   * the handler wants to handle this message, otherwise it can return
   * undefined to let another handler handle the message.
   */
  messageReceived(identifier: string, body: any): Promise<any> | undefined;
}
