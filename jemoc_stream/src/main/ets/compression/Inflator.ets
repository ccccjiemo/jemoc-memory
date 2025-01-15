import { MemoryStream, Inflater } from "libjemoc_stream.so";
import { stream, util } from "@kit.ArkTS";

interface InflatorOption {
  windowBits?: number;
}


export class Inflator {
  private readonly _windowBits?: number;
  private _cache?: MemoryStream;
  private _inflater?: Inflater;
  private _isFinished: boolean;
  private _isDisposed: boolean;
  private _buffer: ArrayBuffer;

  constructor(option?: InflatorOption) {
    this._windowBits = option?.windowBits;
    this._isFinished = false;
    this._isDisposed = false;
    this._cache = new MemoryStream();
    this._inflater = new Inflater(this._windowBits);
    this._buffer = new ArrayBuffer(8196);

  }

  static inflate(chunk: ArrayBufferLike | Uint8Array): Uint8Array {
    let inflater = new Inflator()
    inflater.push(chunk, true);
    let result = inflater.result();
    inflater.dispose();
    return result;
  }

  static async inflaterAsync(chunk: ArrayBufferLike | Uint8Array): Promise<Uint8Array> {
    let inflater = new Inflator();
    await inflater.pushAsync(chunk, true);
    let result = inflater.result();
    inflater._inflater?.dispose();
    inflater._cache?.closeAsync();
    return result;
  }

  static createStream(): InflatorStream {
    return new InflatorStream();
  }


  push(chunk: ArrayBufferLike | Uint8Array, end?: boolean): void {
    this.ensureNotDisposed();
    if (this._isFinished) {
      throw Error('Inflate is finished.');
    }

    this._inflater?.setInput(chunk);
    let readBytes = 0;
    while ((readBytes = this._inflater!.inflate(this._buffer)) != 0) {
      this._cache?.write(this._buffer, 0, readBytes);
    }
    if (end) {
      this._isFinished = true;
    }

  }

  async pushAsync(chunk: ArrayBufferLike | Uint8Array, end?: boolean): Promise<void> {
    this.ensureNotDisposed();
    if (this._isFinished) {
      throw Error('Inflate is finished.');
    }
    this._inflater?.setInput(chunk);
    let readBytes = 0;
    while ((readBytes = this._inflater!.inflate(this._buffer)) != 0) {
      await this._cache?.writeAsync(this._buffer, 0, readBytes);
    }
    if (end) {
      this._isFinished = true;
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
    this._isFinished = true;
    this._inflater?.dispose();
    this._inflater = new Inflater(this._windowBits);
  }

  dispose() {
    if (this._isDisposed) {
      return;
    }
    this._isDisposed = true;
    this._cache?.close();
    this._cache = undefined;
    this._inflater?.dispose();
    this._inflater = undefined;
  }
}

export class InflatorStream extends stream.Transform {
  private _inflater: Inflater;
  private _buffer: Uint8Array;

  constructor(option?: InflatorOption) {
    super();
    this._buffer = new Uint8Array(8196);
    this._inflater = new Inflater(option?.windowBits);
  }

  doTransform(chunk: string, encoding: string, callback: Function): void {
    let encoder = util.TextEncoder.create(encoding == 'utf8' ? 'utf-8' : encoding);
    let buffer = encoder.encodeInto(chunk);
    this._inflater.setInput(buffer);
    let readBytes = 0;
    while ((readBytes = this._inflater.inflate(this._buffer)) != 0) {
      this.push(this._buffer.slice(0, readBytes));
    }
    callback();
  }

  doFlush(callback: Function): void {
    this._inflater.dispose();
    callback();
  }
}