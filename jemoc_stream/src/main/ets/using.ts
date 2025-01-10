import { AsyncCallback, Callback } from '@kit.BasicServicesKit';

interface IDisposable {
  close(): void;
}

interface IAsyncDisposable {
  closeAsync(): Promise<void>;
}

function using<T extends IDisposable>(disposable: T, callback: Callback<T>): void {
  try {
    callback(disposable);
  } finally {
    disposable.close();
  }
}

async function usingAsync<T extends IAsyncDisposable>(disposable: T,
  callback: (target: T) => Promise<void>): Promise<void> {
  try {
    await callback(disposable);
  } finally {
    await disposable.closeAsync();
  }
}

export { using, usingAsync }