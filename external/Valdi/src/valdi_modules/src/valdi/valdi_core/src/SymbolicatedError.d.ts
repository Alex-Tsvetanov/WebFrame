export interface SymbolicatedError extends Error {
  originalStack?: string;
}
