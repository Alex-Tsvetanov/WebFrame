export function wrapper(input: any): () => any {
  return () => {
    return input;
  };
}
