import { MemoryStream, DeflateStream, Deflater } from "libjemoc_stream.so";
import { DeflateStreamMode } from "./DeflateStream";
import { stream, util } from "@kit.ArkTS";

interface DeflatorOption {
  windowBits?: number;
  compressionLevel?: number;
  bufferSize?: number;
}


export class Deflator {
  private readonly _windowBits?: number;
  private readonly _compressionLevel?: number;
  private _cache?: MemoryStream;
  private _deflateStream?: DeflateStream;
  private _isFinished: boolean;
  private _isDisposed: boolean;

  constructor(option?: DeflatorOption) {
    this._windowBits = option?.windowBits;
    this._compressionLevel = option?.compressionLevel;
    this._isFinished = false;
    this._isDisposed = false;
    this._cache = new MemoryStream();
    this._deflateStream = new DeflateStream(this._cache, DeflateStreamMode.Compress, {
      leaveOpen: true,
      windowBits: this._windowBits,
      compressionLevel: this._compressionLevel,
      bufferSize: option?.bufferSize
    })

  }

  static deflate(chunk: ArrayBufferLike | Uint8Array, option?: DeflatorOption): Uint8Array {
    let deflater = new Deflator(option)
    deflater.push(chunk, true);
    let result = deflater.result();
    deflater.dispose();
    return result;
  }

  static async deflateAsync(chunk: ArrayBufferLike | Uint8Array, option?: DeflatorOption): Promise<Uint8Array> {
    let deflater = new Deflator(option);
    await deflater.pushAsync(chunk, true);
    let result = deflater.result();
    deflater._deflateStream?.closeAsync();
    deflater._cache?.closeAsync();
    return result;
  }

  static createStream(option?: DeflatorOption): DeflatorStream {
    return new DeflatorStream(option);
  }


  push(chunk: ArrayBufferLike | Uint8Array, end?: boolean): void {
    this.ensureNotDisposed();
    if (this._isFinished || !this._deflateStream) {
      throw Error('Deflate is finished.');
    }
    this._deflateStream.write(chunk);
    if (end) {
      this._isFinished = true;
      this._deflateStream.close();
      this._deflateStream = undefined;
    }
  }

  async pushAsync(chunk: ArrayBufferLike | Uint8Array, end?: boolean): Promise<void> {
    this.ensureNotDisposed();
    if (this._isFinished || !this._deflateStream) {
      throw Error('Deflate is finished.');
    }
    await this._deflateStream.writeAsync(chunk);
    if (end) {
      this._isFinished = true;
      await this._deflateStream.closeAsync();
      this._deflateStream = undefined;
    }
  }

  get isFinished(): boolean {
    return this._isFinished;
  }

  result(): Uint8Array {
    this.ensureNotDisposed();
    return new Uint8Array(this._cache?.toArrayBuffer());
  }

  private ensureNotDisposed() {
    if (this._isDisposed || !this._cache) {
      throw Error('Deflator is disposed.')
    }
  }

  reset(): void {
    this.ensureNotDisposed();
    if (!this._deflateStream) {
      this._deflateStream = new DeflateStream(this._cache!, DeflateStreamMode.Compress, {
        leaveOpen: true,
        windowBits: this._windowBits,
        compressionLevel: this._compressionLevel
      })
    }
    this._isFinished = false;
    this._cache!.length = 0;
  }

  dispose() {
    if (this._isDisposed) {
      return;
    }
    this._isDisposed = true;
    this._deflateStream?.close();
    this._cache?.close();
    this._deflateStream = undefined;
    this._cache = undefined;
  }
}

export class DeflatorStream extends stream.Transform {
  private _deflater: Deflater;
  private _buffer: Uint8Array;

  constructor(option?: DeflatorOption) {
    super();
    this._buffer = new Uint8Array(8196);
    this._deflater = new Deflater(option?.windowBits, option?.compressionLevel);
  }

  doTransform(chunk: string, encoding: string, callback: Function): void {
    let encoder = util.TextEncoder.create(encoding == 'utf8' ? 'utf-8' : encoding);
    let buffer = encoder.encodeInto(chunk);
    this._deflater.setInput(buffer);
    let readBytes = 0;
    while ((readBytes = this._deflater.deflate(this._buffer)) != 0) {
      this.push(this._buffer.slice(0, readBytes));
    }
    callback();
  }

  doFlush(callback: Function): void {
    let finish: boolean = false;
    do {
      let result = this._deflater.finish(this._buffer);
      finish = result.result;
      if (result.readBytes > 0) {
        this.push(this._buffer.slice(0, result.readBytes))
      }
    } while (!finish);
    this._deflater.dispose();
    callback();
  }
}