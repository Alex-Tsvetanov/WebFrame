function formatValue(value: any): any {
  if (value instanceof Error) {
    return `${value}: ${value.stack}`;
  }
  return value;
}

export function formatMessages(messages: any[]): string {
  return messages.map(formatValue).join(' ');
}
