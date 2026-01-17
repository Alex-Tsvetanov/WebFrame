export enum GptRole {
  USER = 'user',
  ASSISTANT = 'assistant',
  SYSTEM = 'system',
}

export interface GptMessage {
  role: GptRole;
  content: string;
}

export interface AiService {
  addSystemPrompt(message: string): void;
  getResponse(messages: GptMessage[]): Promise<string>;
}
