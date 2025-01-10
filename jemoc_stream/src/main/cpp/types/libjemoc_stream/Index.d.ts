export type BufferLike = ArrayBufferLike | Uint8Array;

export enum SeekOrigin {Begin, Current, End}

export interface DeflateStreamOption {
  leaveOpen?: boolean;
  windowBits?: number;
  uncompressSize?: number;
  bufferSize?: number;
  compressionLevel?: number;
}

export interface IStream {
  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

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

  copyTo(stream: IStream, bufferSize?: number | undefined): void;

  seek(offset: number, origin: number): void;

  read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

  flush(): void;

  close(): void

  get capacity(): number

  set capacity(value: number)

  toArrayBuffer(): ArrayBuffer
}

export class FileStream implements IStream {
  constructor(path: string, mode: number)

  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

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
  constructor(stream: IStream, mode:number, passwd: string, crc: number, option?: ZipCryptoStreamOption)
  
  get canRead(): boolean;

  get canWrite(): boolean;

  get canSeek(): boolean;

  get position(): number;

  get length(): number;

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