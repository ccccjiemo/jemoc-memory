import { stream } from "@kit.ArkTS";
import { fileIo } from "@kit.CoreFileKit";
import { resourceManager } from "@kit.LocalizationKit";
import { ChunkIterator } from "./src/main/ets/base/ChunkIterator";

export type BufferLike = ArrayBufferLike | Uint8Array;

declare namespace base {

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


  /**
   * @since 1.1.2
   */
  interface ToArrayBufferOptions {
    offset?: number;
    length?: number;
  }

  /**
   * Stream基类，所有Stream继承自IStream,不要混用同步和异步方法，异常均会抛出错误
   */
  interface IStream {
    /**
     * 流是否可读
     * @returns
     */
    get canRead(): boolean;

    /**
     * 流是否可写
     * @returns
     */
    get canWrite(): boolean;

    /**
     * 流是否可随机访问
     * @returns
     */
    get canSeek(): boolean;

    /**
     * 流指针位置
     * @returns
     */
    get position(): number;

    /**
     * 流长度
     * @returns
     */
    get length(): number;

    /**
     * 设置流长度，可能会截断流
     * @param value
     */
    set length(value: number);

    get isClosed(): boolean;

    /**
     * 拷贝从指针位置到流末端的数据到指定流（Stream)中.并推动指针到末端
     * @param stream 拷贝接收对象
     * @param bufferSize 拷贝缓冲大小
     */
    copyTo(stream: IStream, bufferSize?: number): void

    /**
     * 拷贝从指针位置到流末端的数据到指定流（Stream)中.并推动指针到末端
     * @param stream 拷贝接收对象
     * @param bufferSize 拷贝缓冲大小
     */
    copyToAsync(stream: IStream, bufferSize?: number): Promise<void>

    /**
     * 随机访问，指定指针位置
     * @param offset 相对偏移
     * @param origin 相对位置
     */
    seek(offset: number, origin: SeekOrigin): void

    /**
     * 从流中读取数据到指定buffer中，并推动指针位置
     * @param buffer 接受buffer
     * @param offset buffer地址偏移。offset不可为负数
     * @param count 读取大小
     * @returns 实际读取大小
     */
    read(buffer: BufferLike, offset?: number, count?: number): number

    /**
     * read的异步方法，从流中读取数据到指定buffer中，并推动指针位置
     * @param buffer 接受buffer
     * @param offset buffer地址偏移。offset不可为负数
     * @param count 读取大小
     * @returns 实际读取大小
     */
    readAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

    /**
     * 将buffer数据写入流中，并推动指针位置
     * @param buffer 要写入的数据
     * @param offset 数据buffer的地址偏移。offset不可为负数
     * @param count 写入大小
     * @returns
     */
    write(buffer: BufferLike, offset?: number, count?: number): number

    /**
     * write异步方法
     */
    writeAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

    /**
     * 刷新流
     */
    flush(): void

    /**
     * flush异步方法
     */
    flushAsync(): Promise<void>

    /**
     * 关闭流对象，并释放流
     */
    close(): void

    /**
     * 关闭流对象，并释放流
     */
    closeAsync(): Promise<void>
  }

  enum SeekOrigin {
    /**
     * 流开始位置
     */
    Begin,
    /**
     * 流当前位置
     */
    Current,
    /**
     * 流末端
     */
    End
  }

  enum FileMode {
    /**
     * 只读
     */
    READ = 0x00,
    /**
     * 只写
     */
    WRITE = 0x01,
    /**
     * 追加模式
     */
    APPEND = 0x02,
    /**
     * 截断模式
     */
    TRUNC = 0x04,
    /**
     * 创建文件如果没有
     */
    CREATE = 0x08
  }

  /**
   * 将base.IStream转换成fs.Stream,方便在rcp中使用（果然没有什么是增加中间层不能解决的）
   * @param stream
   * @returns
   */
  function createFSStream(stream: IStream): fileIo.Stream;

  /**
   * 创建分块迭代器
   * @param stream
   * @returns
   */
  function createStreamChunk(stream: IStream): ChunkIterator;


  /**
   * 文件流，适应IStream方式
   */
  class FileStream implements IStream {
    /**
     *
     * @param path 文件地址需要标准地址，如果是uri请转换成标准地址
     * @param mode 默认Read模式
     */
    constructor(path: string, mode?: FileMode)

    constructor(fd: number, mode?: FileMode)

    constructor(rawFile: resourceManager.RawFileDescriptor)

    get isClosed(): boolean;

    /**
     * 当追加模式，这个是可能是false
     * @returns
     */
    get canRead(): boolean;

    /**
     * 只读时，这个值可能是false
     * @returns
     */
    get canWrite(): boolean;

    /**
     * 追加模式时，这个时可能是false
     * @returns
     */
    get canSeek(): boolean;

    /**
     * 文件流指针位置
     * @returns
     */
    get position(): number;

    /**
     * 文件大小
     * @returns
     */
    get length(): number;

    /**
     * 设置大小，可用于截断文件
     * @param value
     */
    set length(value: number);

    /**
     * 将文件流当前指针位置到文件流末尾的数据拷贝到目标中，并推动指针位置
     * @param stream
     * @param bufferSize
     */
    copyTo(stream: IStream, bufferSize?: number | undefined): void;

    /**
     * 将文件流当前指针位置到文件流末尾的数据拷贝到目标中，并推动指针位置
     * @param stream
     * @param bufferSize
     */
    copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

    /**
     * 当canSeek为true时，可以使用方法改变指针位置
     * @param offset
     * @param origin
     */
    seek(offset: number, origin: SeekOrigin): void;

    /**
     * 读取数据
     */
    read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    /**
     * 读取数据
     */
    readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    flush(): void;

    flushAsync(): Promise<void>;

    close(): void;

    closeAsync(): Promise<void>;
  }

  /**
   * 内存流，自动扩容
   */
  class MemoryStream implements IStream {
    constructor(capacity: number)

    constructor(buffer: BufferLike)

    constructor()

    get canRead(): boolean;

    get canWrite(): boolean;

    get canSeek(): boolean;

    get position(): number;

    get length(): number;

    set length(value: number);

    get isClosed(): boolean;

    copyTo(stream: IStream, bufferSize?: number | undefined): void;

    copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

    seek(offset: number, origin: SeekOrigin): void;

    read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    flush(): void;

    flushAsync(): Promise<void>;

    close(): void;

    closeAsync(): Promise<void>;

    /**
     * 返回流中的所有数据，不修改指针位置
     * @returns
     */
    toArrayBuffer(options?: ToArrayBufferOptions): ArrayBuffer;
  }

  export interface SendFileOptions {
    offset?: number
    length?: number
    autoClose?: boolean
  }

  class MemfdStream implements IStream {
    constructor(buffer?: BufferLike)

    get canRead(): boolean;

    get canWrite(): boolean;

    get canSeek(): boolean;

    get position(): number;

    get length(): number;

    set length(value: number);

    get isClosed(): boolean;

    copyTo(stream: IStream, bufferSize?: number | undefined): void;

    copyToAsync(stream: IStream, bufferSize?: number | undefined): Promise<void>;

    seek(offset: number, origin: SeekOrigin): void;

    read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    flush(): void;

    flushAsync(): Promise<void>;

    close(): void;

    closeAsync(): Promise<void>;

    /**
     * 返回流中的所有数据，不修改指针位置
     * @returns
     */
    toArrayBuffer(options?: ToArrayBufferOptions): ArrayBuffer;

    /**
     * 获取文件描述符
     * @returns
     */
    get fd(): number;

    /**
     * 将数据保存到fd
     * @param fd
     */
    sendFile(fd: number, options?: SendFileOptions): boolean
    sendFile(path: string, mode: number, options?: SendFileOptions): boolean;

    sendFileAsync(fd: number, options?: SendFileOptions): Promise<boolean>
    sendFileAsync(path: string, mode: number, options?: SendFileOptions): Promise<boolean>
  }


}

declare namespace compression {

  /**
   * @since 1.1.2
   */
  export enum BrotliStreamMode {
    Compress,
    Decompress
  }

  /**
   * @since 1.1.2
   */
  export interface BrotliStreamOptions {
    quality?: number;
    lgWin?: number;
    mode?: number;
    leaveOpen?: boolean;
    bufferSize?: number;
  }

  /**
   * @since 1.1.2
   */
  export class BrotliStream implements base.IStream {
    constructor(stream: base.IStream, mode: BrotliStreamMode, options?: BrotliStreamOptions)

    get canRead(): boolean;

    get canWrite(): boolean;

    get canSeek(): boolean;

    get position(): number;

    get length(): number;

    set length(value: number);

    get isClosed(): boolean;


    copyTo(stream: base.IStream, bufferSize?: number | undefined): void;

    copyToAsync(stream: base.IStream, bufferSize?: number | undefined): Promise<void>;

    seek(offset: number, origin: base.SeekOrigin): void;

    read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    flush(): void;

    flushAsync(): Promise<void>;

    close(): void;

    closeAsync(): Promise<void>;
  }

  /**
   * @since 1.1.2
   */
  export interface BrotliConfig {
    quality?: number;
    lgWin?: number;
    mode?: number;
  }

  /**
   * @since 1.1.2
   */
  export namespace BrotliUtils {

    export function compress(buffer: BufferLike | string, config?: BrotliConfig): ArrayBuffer | undefined;

    export function decompress(buffer: BufferLike | string): ArrayBuffer | undefined;

    export function compressAsync(buffer: BufferLike | string, config?: BrotliConfig): Promise<ArrayBuffer>

    export function decompressAsync(buffer: BufferLike | string): Promise<ArrayBuffer>;

  }

  export enum DeflateStreamMode {
    /**
     * 压缩模式
     */
    Compress,
    /**
     * 解压模式
     */
    Decompress
  }

  interface DeflateStreamOption {
    leaveOpen?: boolean;
    windowBits?: number;
    uncompressSize?: number;
    bufferSize?: number;
    compressionLevel?: number;
  }

  /**
   * 压缩等级
   */
  namespace CompressionLevel {
    /**
     * 不压缩
     */
    export const NO_COMPRESSION = 0;

    /**
     * 最大压缩
     */
    export const BEST_COMPRESSION = 9;

    /**
     * 最快
     */
    export const BEST_SPEED = 1;

    /**
     * 默认压缩等级
     */
    export const DEFAULT_COMPRESSION = -1;
  }

  /**
   * 压缩策略,详情见zlib
   */
  namespace CompressionStrategy {
    export const DEFAULT_STRATEGY = 0;

    export const FILTERED = 1;

    export const HUFFMAN_ONLY = 2;

    export const RLE = 3;

    export const FIXED = 4;
  }


  /**
   * Deflate流,底层使用zlib-ng实现deflate、inflate;
   *
   * 学习C#的DeflateStream
   */
  class DeflateStream implements base.IStream {
    constructor(stream: base.IStream, mode: DeflateStreamMode, option?: DeflateStreamOption)

    get isClosed(): boolean;

    /**
     * 解压模式可读
     * @returns
     */
    get canRead(): boolean;

    /**
     * 压缩模式可写
     * @returns
     */
    get canWrite(): boolean;

    /**
     * 所有模式不可随机访问
     * @returns
     */
    get canSeek(): boolean;

    get position(): number;

    /**
     * 所有模式无法获取长度
     * @returns
     */
    get length(): number;

    // /**
    //  * 所有模式不可设置流长度
    //  * @param value
    //  */
    // set length(value: number);

    /**
     * 解压模式可用
     * @param stream
     * @param bufferSize
     */
    copyTo(stream: base.IStream, bufferSize?: number | undefined): void;

    /**
     * 解压模式可用
     * @param stream
     * @param bufferSize
     */
    copyToAsync(stream: base.IStream, bufferSize?: number | undefined): Promise<void>;

    /**
     * 所有模式不可用
     * @param offset
     * @param origin
     */
    seek(offset: number, origin: base.SeekOrigin): void;

    /**
     * 获取解压数据
     */
    read(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    /**
     * 获取解压数据
     */
    readAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    /**
     * 写入待压缩数据
     */
    write(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): number;

    /**
     * 写入待压缩数据
     */
    writeAsync(buffer: BufferLike, offset?: number | undefined, count?: number | undefined): Promise<number>;

    /**
     * 压缩模式需要刷新缓冲
     */
    flush(): void;

    /**
     * 压缩模式需要刷新缓冲
     */
    flushAsync(): Promise<void>;

    close(): void;

    closeAsync(): Promise<void>;
  }

  interface DeflatorOption {
    windowBits?: number;
    compressionLevel?: number;
    bufferSize?: number;
  }

  /**
   * Deflate压缩工具，gzip请设置windowBits值
   */
  class Deflator {
    /**
     * windowBits默认-15， zlib无压缩头
     * compressionLevel默认DEFAULT_COMPRESSION
     * @param option
     */
    constructor(option?: DeflatorOption)

    /**
     * deflate压缩，直接获取结果
     */
    static deflate(chunk: BufferLike, option?: DeflatorOption): Uint8Array

    /**
     * deflate的异步方法
     */
    static deflateAsync(chunk: ArrayBufferLike | Uint8Array, option?: DeflatorOption): Promise<Uint8Array>

    /**
     * 返回deflate转换流，继承自鸿蒙stream.Transform
     */
    static createStream(option?: DeflatorOption): DeflatorStream

    /**
     * 写入数据，end为true时结束
     */
    push(chunk: BufferLike, end?: boolean): void

    /**
     * push异步方法
     */
    pushAsync(chunk: BufferLike, end?: boolean): Promise<void>

    /**
     * 是否已经设置结束了
     */
    get isFinished(): boolean

    /**
     * 获取压缩结果，push在end为真结束后，能获取最终结果，因为内部使用Z_NO_FLUSH,未结束时可能不会有数据
     * @returns
     */
    result(): Uint8Array

    /**
     * 重置Deflator，可以重新写入数据并压缩
     */
    reset(): void

    /**
     * 释放Deflator,释放后，无法reset
     */
    dispose(): void
  }

  /**
   * 这个和DeflateStream长得有点像，继承自鸿蒙stream.Transform
   *
   * ps:鸿蒙给的stream是真不好用
   */
  class DeflatorStream extends stream.Transform {
    constructor(option?: DeflatorOption)
  }

  interface InflatorOption {
    windowBits?: number;
  }

  /**
   * Deflate解压缩工具，
   */
  class Inflator {
    /**
     * windowBits默认-15 无zlib头。gzip请设置windowBits值
     */
    constructor(option?: InflatorOption)

    /**
     * deflate解压缩，直接获取结果
     */
    static inflate(chunk: ArrayBufferLike | Uint8Array): Uint8Array

    /**
     * inflate异步方法
     */
    static inflaterAsync(chunk: ArrayBufferLike | Uint8Array): Promise<Uint8Array>

    /**
     * Inflate转换流，继承自鸿蒙stream.Transform
     */
    static createStream(): InflatorStream

    /**
     * 写入数据， end为true时结束
     */
    push(chunk: ArrayBufferLike | Uint8Array, end?: boolean): void

    /**
     * 写入数据， end为true时结束
     */
    pushAsync(chunk: ArrayBufferLike | Uint8Array, end?: boolean): Promise<void>

    /**
     * 是否已经设置结束了
     */
    get isFinished(): boolean

    /**
     * 获取解压结果
     */
    result(): Uint8Array

    /**
     * 重置Inflator
     */
    reset(): void

    /**
     * 释放Inflator
     */
    dispose(): void
  }

  class InflatorStream extends stream.Transform {
    constructor(option?: InflatorOption)
  }

  enum ZipArchiveMode {
    Read, Update, Create
  }

  interface ZipArchiveOption {
    mode?: ZipArchiveMode;
    leaveOpen?: boolean;
    password?: string;
  }

  /**
   * zip压缩包，所有方法请使用try catch捕获错误
   *
   */
  export class ZipArchive {
    /**
     * 从数据流中打开zip压缩包，
     * ZipArchive打开模式有Read只读模式，Update更新模式， Create创建模式，
     * 设置leaveOpen，关闭压缩包时是否关闭流，
     * 设置password, 当文件解压或压缩需要加密时
     * */
    constructor(stream: base.IStream, option?: ZipArchiveOption)

    /**
     * 从文件路径打开zip压缩包，
     * ZipArchive打开模式有Read只读模式，Update更新模式， Create创建模式，
     * 设置leaveOpen，关闭压缩包时是否关闭流，
     * 设置password, 当文件解压或压缩需要加密时
     * */
    constructor(path: string, option?: ZipArchiveOption)

    /**
     * 只读方式读取读取rawFile
     * @param rawFile
     */
    constructor(rawFile: resourceManager.RawFileDescriptor, password?: string)

    /**
     * 获取Zip所有entry
     * @returns
     */
    get entries(): ZipArchiveEntry[]

    /**
     * zip comment
     * @returns
     */
    get comment(): string

    set comment(value: string)

    /**
     * 获取打开模式
     * @returns
     */
    get mode(): number

    /**
     * 通过entry名称获取entry
     * @param entryName
     * @returns
     */
    getEntry(entryName: string): ZipArchiveEntry | undefined

    /**
     * 创建entry，只读模式会报错
     * @param entryName entry名称
     * @param compressionLevel 压缩等级
     * @returns
     */
    createEntry(entryName: string, compressionLevel?: number): ZipArchiveEntry

    get entryNames(): string[]

    get isClosed(): boolean

    /**
     * 关闭压缩包并释放
     */
    close(): void
  }

  /**
   * ZipArchive的entry对象， ZipArchiveEntry提供的设置方法，在open后不可使用。请使用try catch避免因抛出错误导致程序闪退
   */
  export class ZipArchiveEntry {
    private constructor()

    /**
     * 打开数据流，用于写入或读取数据。
     * ZipArchive在Read模式只能用于读取
     * @returns
     */
    open(): base.IStream

    /**
     * 删除ZipArchive中的entry
     */
    delete(): void

    /**
     * entry名称
     * @returns
     */
    get fullName(): string

    /**
     * 设置entry名称
     * @param value
     */
    set fullName(value: string)

    /**
     * 文件是否加密，目前仅支持ZipCrypto
     * @returns
     */
    get isEncrypted(): boolean

    /**
     * 设置是否加密文件，目前仅支持ZipCrypto
     * @param value
     */
    set isEncrypted(value: boolean)

    /**
     * 获取压缩等级
     */
    get compressionLevel(): number

    /**
     * 设置压缩等级, No_Compression时为Store模式不压缩文件
     */
    set compressionLevel(value: number)


    get fileComment(): string

    set fileComment(value: string)

    get lastModifier(): Date

    get crc32(): number

    /**
     * 是否已经打开流
     */
    get isOpened(): boolean

    /**
     * 是否已经删除，它不一定是删除，有可能是native对象已经释放了，但js对象因为引用关系还未释放
     * @returns
     */
    get isDeleted(): boolean

    /**
     * ZipArchive打开使用Read模式或者在Update模式下未open，可以获取到值，否则返回0
     * @returns
     */
    get uncompressedSize(): number

    /**
     * ZipArchive打开使用Read模式或者在Update模式下未open，可以获取到值，否则返回0
     * @returns
     */
    get compressedSize(): number
  }
}

/**
 * BufferPool模块
 */
declare namespace bufferpool {
  export interface BufferPoolStats {
    total: number
    used: number
  }

  export abstract class BufferPool {
    abstract acquire(size: number): ArrayBuffer

    abstract release(buffer: ArrayBuffer): void

    get stats(): BufferPoolStats

    protected updateStats(acquiring: boolean): void
  }

  /**
   * LRU
   */
  export class LruBufferPool extends BufferPool {
    constructor(maxSize: number)

    acquire(size: number): ArrayBuffer;

    release(buffer: ArrayBuffer): void;
  }
}

/**
 * 用于桥接@jemoc/stream与官方stream工具
 */
declare namespace streamUtils {
  /**
   * 将可读IStream转换成stream.Readable
   * @param stream
   * @returns
   */
  export function streamToReadable(stream: base.IStream): stream.Readable

  /**
   * 将可写流转换成stream.Writable
   * @param stream
   * @returns
   */
  export function streamToWriteable(stream: base.IStream): stream.Writable

  /**
   * 用于多路可写流，将可读流同时写入多个可写流
   * @param stream
   * @returns
   */
  export function createMultiWritable(...stream: base.IStream[]): stream.Writable
}

declare namespace reader {
  export abstract class TextReader {
    abstract read(buffer?: ArrayBuffer): number;

    abstract readLine(): string;
  }

  export interface StreamReaderOptions {
    detectEncodingFromByteOrderMarks?: boolean;
    encoding?: string;
    leaveOpen?: boolean
  }

  export class StreamReader extends TextReader {
    constructor(target: string | base.IStream, options?: StreamReaderOptions)

    readToEnd(): string;

    skip(count: number): number;

    peek(): number;

    close: void;

    read(buffer?: ArrayBuffer | undefined): number;

    readLine(): string;

    get encoding(): string;

    get endOfStream(): boolean;
  }

  export class XmlReader {
    constructor(target: string | TextReader)

    read(): boolean;

    get name(): string;

    get value(): string;

    get nodeType(): XmlNodeType;

    getAttribute(attribute: string): string;

    get attributes(): Record<string, string>

    get isEmptyElement(): boolean;

    close(): void;
  }

  export enum XmlNodeType {
    None,
    Element,
    Attribute,
    Text,
    CDATA,
    Comment,
    Document,
    EndElement,
    XmlDeclaration,
    DocumentType,
    ProcessingInstruction,
    Whitespace
  }
}