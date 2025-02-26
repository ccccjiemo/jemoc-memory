import { resourceManager } from "@kit.LocalizationKit";

export type BufferLike = ArrayBufferLike | Uint8Array;

export enum SeekOrigin { Begin, Current, End }

export interface ToArrayBufferOptions {
  offset?: number;
  length?: number;
}

export interface DeflateStreamOption {
  leaveOpen?: boolean;
  windowBits?: number;
  uncompressSize?: number;
  bufferSize?: number;
  compressionLevel?: number;
}

export interface BufferPoolStats {
  total: number
  used: number
}

declare abstract class BufferPool {
  abstract acquire(size: number): ArrayBuffer

  abstract release(buffer: ArrayBuffer): void

  get stats(): BufferPoolStats

  protected updateStats(acquiring: boolean): void
}

export class LruBufferPool extends BufferPool {
  constructor(maxSize: number)

  acquire(size: number): ArrayBuffer;

  release(buffer: ArrayBuffer): void;
}

export abstract class StreamBase {
  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

  set length(value: number);

  get isClosed(): boolean;

  copyTo(stream: IStream, bufferSize?: number): void

  copyToAsync(stream: IStream, bufferSize?: number): Promise<void>

  seek(offset: number, origin: number): void

  read(buffer: BufferLike, offset?: number, count?: number): number

  readAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

  write(buffer: BufferLike, offset?: number, count?: number): number

  writeAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

  flush(): void

  flushAsync(): Promise<void>

  close(): void

  closeAsync(): Promise<void>
}

export interface IStream {

  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

  set length(value: number);

  copyTo(stream: IStream, bufferSize?: number): void

  copyToAsync(stream: IStream, bufferSize?: number): Promise<void>

  seek(offset: number, origin: number): void

  read(buffer: BufferLike, offset?: number, count?: number): number

  readAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

  write(buffer: BufferLike, offset?: number, count?: number): number

  writeAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

  flush(): void

  flushAsync(): Promise<void>

  close(): void

  closeAsync(): Promise<void>
}

export class MemoryStream implements IStream {
  constructor(capacity: number)

  constructor(buffer: BufferLike)

  constructor()

  copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

  readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  flushAsync(): Promise<void>;

  closeAsync(): Promise<void>;

  get canSeek(): boolean;

  get canRead(): boolean;

  get canWrite(): boolean;

  get position(): number;

  get length(): number;

  set length(value: number);

  copyTo(stream: IStream, bufferSize?: number | undefined): void;

  seek(offset: number, origin: number): void;

  read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  flush(): void;

  close(): void

  get capacity(): number

  set capacity(value: number)

  toArrayBuffer(options?: ToArrayBufferOptions): ArrayBuffer
}

export class FileStream implements IStream {
  constructor(path: string, mode?: number)

  constructor(fd: number, mode?: number)

  constructor(rawFile: resourceManager.RawFileDescriptor)


  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

  set length(value: number);

  copyTo(stream: IStream, bufferSize?: number | undefined): void;

  copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

  seek(offset: number, origin: number): void;

  read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  flush(): void;

  flushAsync(): Promise<void>;

  close(): void;

  closeAsync(): Promise<void>;
}

export class DeflateStream implements IStream {
  constructor(stream: IStream, mode: number, option?: DeflateStreamOption)

  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

  set length(value: number);

  copyTo(stream: IStream, bufferSize?: number | undefined): void;

  copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

  seek(offset: number, origin: number): void;

  read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  flush(): void;

  flushAsync(): Promise<void>;

  close(): void;

  closeAsync(): Promise<void>;
}

interface ZipCryptoStreamOption {
  leaveOpen?: boolean;
  bufferSize?: number;
}

export class ZipCryptoStream implements IStream {
  constructor(stream: IStream, mode: number, passwd: string, crc: number, option?: ZipCryptoStreamOption)

  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

  set length(value: number);

  copyTo(stream: IStream, bufferSize?: number | undefined): void;

  copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

  seek(offset: number, origin: number): void;

  read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  flush(): void;

  flushAsync(): Promise<void>;

  close(): void;

  closeAsync(): Promise<void>;
}

interface ZipArchiveOption {
  mode?: number;
  leaveOpen?: boolean;
  password?: string;
}

export class ZipArchiveEntry {
  private constructor()

  open(): IStream

  delete(): void

  get fullName(): string

  set fullName(value: string)

  get isEncrypted(): boolean

  set isEncrypted(value: boolean)

  get compressionLevel(): number

  set compressionLevel(value: number)

  get compressionMethod(): number

  set compressionMethod(value: number)

  get fileComment(): string

  set fileComment(value: string)

  get lastModifier(): Date

  get crc32(): number

  get isOpened(): boolean

  get isDeleted(): boolean

  get uncompressedSize(): number

  get compressedSize(): number;
}

export class ZipArchive {
  constructor(stream: IStream, option?: ZipArchiveOption)

  constructor(path: string, option?: ZipArchiveOption)

  get entries(): ZipArchiveEntry[]

  get comment(): string

  set comment(value: string)

  get mode(): number

  getEntry(entryName: string): ZipArchiveEntry | undefined

  createEntry(entryName: string, compressionLevel?: number): ZipArchiveEntry

  close(): void

  get entryNames(): string[]
}


export class Deflater {
  constructor(windowBits?: number, compressionLevel?: number, strategy?: number)

  setInput(input: BufferLike, offset?: number, count?: number): void

  dispose(): void

  deflate(buffer: BufferLike, offset?: number, count?: number): number

  flush(buffer: BufferLike, offset?: number, count?: number): { result: boolean, readBytes: number }

  finish(buffer: BufferLike, offset?: number, count?: number): { result: boolean, readBytes: number }

  get needInput(): boolean

  get isDisposed(): boolean
}

export class Inflater {
  constructor(windowBits?: number, uncompressSize?: number)

  setInput(input: BufferLike, offset?: number, count?: number): void

  dispose(): void

  inflate(buffer: BufferLike, offset?: number, count?: number): number

  get needInput(): boolean

  get isDisposed(): boolean

  get isFinished(): boolean

  get isGzipInput(): boolean
}

export interface SendFileOptions {
  offset?: number
  length?: number
  autoClose?: boolean
}

export class MemfdStream implements IStream {
  constrructor(buffer: BufferLike)

  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

  set length(value: number);

  copyTo(stream: IStream, bufferSize?: number | undefined): void;

  copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

  seek(offset: number, origin: number): void;

  read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

  flush(): void;

  flushAsync(): Promise<void>;

  close(): void;

  closeAsync(): Promise<void>;

  toArrayBuffer(options?: ToArrayBufferOptions): ArrayBuffer;

  get fd(): number;

  sendFile(fd: number, options?: SendFileOptions): boolean

  sendFile(path: string, mode: number, options?: SendFileOptions): boolean

  sendFileAsync(fd: number, options?: SendFileOptions): Promise<boolean>

  sendFileAsync(path: string, mode: number, options?: SendFileOptions): Promise<boolean>
}

export interface BrotliStreamOptions {
  quality?: number;
  lgWin?: number;
  mode?: number;
  leaveOpen?: boolean;
  bufferSize?: number;
}

export class BrotliStream extends StreamBase {
  constructor(stream: IStream, mode: number, options?: BrotliStreamOptions)
}

export interface BrotliConfig {
  quality?: number;
  lgWin?: number;
  mode?: number;
}

export class BrotliUtils {
  static compress(buffer: BufferLike | string, config?: BrotliConfig): ArrayBuffer | undefined;

  static decompress(buffer: BufferLike | string): ArrayBuffer | undefined;

  static compressAsync(buffer: BufferLike | string, config?: BrotliConfig): Promise<ArrayBuffer>;

  static decompressAsync(buffer: BufferLike | string): Promise<ArrayBuffer>
}

// export abstract class TextReader {
//   abstract read(buffer?: ArrayBuffer): number;
//
//   abstract readLine(): string;
//
//   abstract readToEnd(): string;
//
//   abstract skip(count: number): number;
//
//   abstract peek(): number;
//
//   abstract close: void;
// }
//
// export class StreamReader extends TextReader {
//   constructor(target: string | IStream, { detectEncodingFromByteOrderMarks: boolean, encoding: string })
//
//   readToEnd(): string;
//
//   skip(count: number): number;
//
//   peek(): number;
//
//   close: void;
//
//   read(buffer?: ArrayBuffer | undefined): number;
//
//   readLine(): string;
//
//   get encoding(): string;
// }
//
// export class XmlReader {}

export namespace reader {
  export abstract class TextReader {
    abstract read(buffer?: ArrayBuffer): number;

    abstract readLine(): string;

    abstract readToEnd(): string;

    abstract skip(count: number): number;

    abstract peek(): number;

    abstract close: void;
  }

  export class StreamReader extends TextReader {
    constructor(target: string | IStream, { detectEncodingFromByteOrderMarks: boolean, encoding: string })

    readToEnd(): string;

    skip(count: number): number;

    peek(): number;

    close: void;

    read(buffer?: ArrayBuffer | undefined): number;

    readLine(): string;

    get encoding(): string;
  }

  export class XmlReader {}
}