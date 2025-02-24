export interface Disposeable {
  close(): void;

  closeAsync(): Promise<void>;
}