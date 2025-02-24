export interface Closable {
  close(): void;

  closeAsync(): Promise<void>
}

export function using<T extends Closable>(
  target: T,
  callback: (target: T) => Promise<void>
): Promise<void>;

export function using<T extends Closable>(
  target: T,
  callback: (target: T) => void
): void;

export function using<T extends Closable>(
  target: T,
  callback: (target: T) => void | Promise<void>
): void | Promise<void> {
  const result = callback(target);

  if (result instanceof Promise) {
    // 处理异步回调
    return result.finally(async () => {
      await target.closeAsync();
    });
  } else {
    // 处理同步回调
    try {
      return result;
    } finally {
      target.close();
    }
  }
}
