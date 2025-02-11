import { IStream } from 'libjemoc_stream.so'
import { fileIo as fs, ReadOptions, WriteOptions } from '@kit.CoreFileKit'
import { AsyncCallback, BusinessError } from '@ohos.base'
import { util } from '@kit.ArkTS';

export enum SeekOrigin { Begin, Current, End }


export function createFSStream(stream: IStream): fs.Stream {
  return new OHStream(stream);
}

class OHStream implements fs.Stream {
  private stream: IStream;

  constructor(stream: IStream) {
    this.stream = stream;
  }

  close(): Promise<void>
  close(callback: AsyncCallback<void, void>): void
  close(callback?: AsyncCallback<void, void>): void | Promise<void> {
    if (callback) {
      try {
        this.stream.close();
      } catch (e) {
      } finally {
        callback(undefined);
      }

    } else {
      return this.stream.closeAsync();
    }
  }

  closeSync(): void {
    this.stream.close();
  }

  flush(): Promise<void>
  flush(callback: AsyncCallback<void, void>): void
  flush(callback?: AsyncCallback<void, void>): void | Promise<void> {
    if (callback) {
      this.stream.flush();
    } else {
      return this.stream.flushAsync();
    }
  }

  flushSync(): void {
    this.stream.flush();
  }

  write(buffer: string | ArrayBuffer, options?: WriteOptions | undefined): Promise<number>
  write(buffer: string | ArrayBuffer, callback: AsyncCallback<number, void>): void
  write(buffer: string | ArrayBuffer, options: WriteOptions, callback: AsyncCallback<number, void>): void
  write(buffer: string | ArrayBuffer, options?: (WriteOptions | undefined) | WriteOptions | AsyncCallback<number, void>,
    callback?: AsyncCallback<number, void>): void | Promise<number> {

    let func: AsyncCallback<number, void> | undefined;
    let encoding: string = 'utf-8';
    let offset: undefined | number;
    let length: undefined | number;
    if (typeof options !== 'function') {
      encoding = options?.encoding ?? 'utf-8';
      offset = options?.offset;
      length = options?.length;
    } else {
      func = options;
    }

    const data = typeof buffer === 'string' ? util.TextEncoder.create(encoding).encodeInto(buffer) : buffer;

    if (typeof offset != undefined) {
      this.stream.seek(offset, SeekOrigin.Begin);
    }

    if (!func && typeof callback === 'function') {
      func = callback;
    }

    if (func) {
      try {
        const writeBytes = this.stream.write(data, 0, clamp(length, 0, data.byteLength));
        func(undefined, writeBytes);
      } catch (e) {
        func(handleError(13900042, e), 0)
      }
    } else {
      return this.stream.writeAsync(data, 0, clamp(length, 0, data.byteLength));
    }

  }

  writeSync(buffer: string | ArrayBuffer, options?: WriteOptions | undefined): number {
    if (typeof options?.offset === 'number') {
      this.stream.seek(options.offset, SeekOrigin.Begin);
    }
    const data: ArrayBuffer =
      typeof buffer === 'string' ? util.TextEncoder.create(options?.encoding ?? 'utf-8').encodeInto(buffer) : buffer;

    const length = clamp(options?.length, 0, data.byteLength);
    return this.stream.write(data, 0, length);
  }

  read(buffer: ArrayBuffer, options?: ReadOptions | undefined): Promise<number>
  read(buffer: ArrayBuffer, callback: AsyncCallback<number, void>): void
  read(buffer: ArrayBuffer, options: ReadOptions, callback: AsyncCallback<number, void>): void
  read(buffer: ArrayBuffer, options?: (ReadOptions | undefined) | ReadOptions | AsyncCallback<number, void>,
    callback?: AsyncCallback<number, void>): void | Promise<number> {


    if (typeof options === 'function') {
      try {
        const readBytes = this.stream.read(buffer);
        options(undefined, readBytes);
      } catch (e) {
        options(handleError(13900042, e), 0);

      }
    } else {
      if (typeof options?.offset === 'number') {
        this.stream.seek(options.offset, SeekOrigin.Begin);
      }
      const length = clamp(options?.length, 0, buffer.byteLength);
      if (!callback) {
        return this.stream.readAsync(buffer, 0, length);
      } else {
        try {
          const readBytes = this.stream.read(buffer, 0, length);
          callback(undefined, readBytes);
        } catch (e) {
          callback(handleError(13900042, e), 0)
        }
      }
    }


  }

  readSync(buffer: ArrayBuffer, options?: ReadOptions | undefined): number {
    if (typeof options?.offset === 'number') {
      this.stream.seek(options.offset, SeekOrigin.Begin);
    }
    const length = clamp(options?.length, 0, buffer.byteLength);
    return this.stream.read(buffer, 0, length);
  }
}

function clamp(value: number | undefined, min: number, max: number): number {
  // 如果 value 是 undefined，返回最大值
  if (value === undefined) {
    return max;
  }
  // 否则，确保 value 在 min 和 max 之间
  return Math.min(Math.max(value, min), max);
}

function handleError(code: number, e: ESObject): BusinessError {
  return {
    code: code,
    name: '',
    message: e
  }
}