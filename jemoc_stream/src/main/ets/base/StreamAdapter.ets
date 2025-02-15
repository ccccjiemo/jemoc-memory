import { stream, util } from '@kit.ArkTS'
import { IStream } from './IStream';


export class ReadableAdapter extends stream.Readable {
  private source: IStream;

  constructor(source: IStream) {
    if (!source.canRead) {
      throw new TypeError('Cannot create ReadableAdapter from a non-readable stream');
    }
    super();
    this.source = source;
    this.on('close', () => {
      this.source.close();
    })
  }

  doRead(size: number): void {
    //后面改用缓存池
    const buffer = new Uint8Array(size);
    try {
      const actualRead = this.source.read(buffer);
      if (actualRead) {
        this.push(buffer.subarray(0, actualRead))
      } else {
        this.push(null);
      }
    } catch (err) {
      this.source.close();
    }
  }
}

export class WriteableAdapter extends stream.Writable {
  private baseStream: IStream;

  constructor(stream: IStream) {
    if (!stream.canWrite) {
      throw new TypeError('Cannot create WriteableAdapter from a non-writeable stream');
    }
    super();
    this.baseStream = stream;
    this.on('close', () => {
      stream.close();
    })
  }

  doWrite(chunk: string | Uint8Array, encoding: string, callback: Function): void {
    try {
      let buffer: Uint8Array;
      if (typeof chunk === 'string') {
        buffer = util.TextEncoder.create(encoding === 'utf8' ? 'utf-8' : encoding).encodeInto(chunk);
      } else {
        buffer = chunk;
      }
      this.baseStream.write(buffer);
    } catch (e) {
      callback(e);
    }
  }
}

export class MultiWriteable extends stream.Writable {
  private targets: stream.Writable[];

  constructor(...streams: stream.Writable[]) {
    super();
    this.targets = streams;
  }

  doWrite(chunk: string | Uint8Array, encoding: string, callback: Function): void {
    this.targets.forEach(stream => {
      stream.write(chunk, encoding, callback);
    })
  }
}


export namespace streamUtils {
  export function streamToReadable(stream: IStream): stream.Readable {
    return new ReadableAdapter(stream);
  }

  export function streamToWriteable(stream: IStream): stream.Writable {
    return new WriteableAdapter(stream);
  }

  export function createMultiWritable(...stream: IStream[]): stream.Writable {
    return new MultiWriteable(...stream.map(streamToWriteable))
  }

}
