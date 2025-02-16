import { IStream } from './IStream'

const chunkSize: number = 1024 * 8;

/**
 * 分块迭代器实现
 * @param stream
 * @returns
 */

export class ChunkIterator implements AsyncIterable<Uint8Array>, Iterable<Uint8Array> {
  private baseStream: IStream;
  private chunkSize: number;

  constructor(stream: IStream, chunkSize?: number) {
    if (!stream.canRead) {
      throw new Error('Stream is not readable');
    }
    this.baseStream = stream;
    this.chunkSize = Math.max(1, chunkSize ?? chunkSize);
  }

  async * [Symbol.asyncIterator]() {
    let buffer = new Uint8Array(this.chunkSize);
    let actualRead: number;

    while (true) {
      try {
        actualRead = await this.baseStream.readAsync(buffer);
        if (actualRead <= 0) {
          break;
        }
      } catch (e) {
        throw new Error(`Async stream read failed: ${e.message}`);
      }
      // Directly use the buffer view with actual read bytes
      yield buffer.subarray(0, actualRead);
    }
  }

  * [Symbol.iterator](): IterableIterator<Uint8Array> {
    let buffer = new Uint8Array(this.chunkSize);
    let actualRead: number;

    while (true) {
      try {
        actualRead = this.baseStream.read(buffer);
        if (actualRead <= 0) {
          break;
        }
      } catch (e) {
        throw new Error(`Sync stream read failed: ${e.message}`);
      }
      // Directly use the buffer view with actual read bytes
      yield buffer.subarray(0, actualRead);
    }
  }
}

export function createStreamChunk(stream: IStream, chunkSize?: number) {
  return new ChunkIterator(stream, chunkSize);
}
