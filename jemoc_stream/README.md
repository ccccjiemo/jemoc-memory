## @jemoc/stream

---
方法参考.net Stream。zlib-ng提供Inflate、Deflate支持。
gzip加解压缩通过设置windowBits实现。所有方法windowBits默认-15

### 如何安装

___

```shell
ohpm install @jemoc/stream
```

## 目录

--- 

- [基础流 (命名空间 base)](#基础流-namespace-base)
    - [IStream 接口](#istream-接口)
    - [BufferLike 类型](#bufferlike-类型)
    - [SeekOrigin 枚举](#seekorigin-枚举)
    - [FileMode 枚举](#filemode-枚举)
    - [FileStream 类](#filestream-类)
    - [MemoryStream 类](#memorystream-类)
    - [MemfdStream 类](#memfdstream-类)
    - [createFSStream 方法](#createfsstream-方法)
    - [createStreamChunk 方法](#createstreamchunk-方法)
- [流工具 (命名空间 streamUtils)](#流工具-namespace-streamutils)
    - [streamToReadable 方法](#streamtoreadable-方法)
    - [streamToWriteable 方法](#streamtoreadable-方法)
    - [createMultiWritable 方法](#createmultiwritable)
- [压缩流 (命名空间 compression)](#压缩流-namespace-compression)
    - [DeflateStream 类](#deflatestream-类)
    - [BrotliStream 类](#brotlistream)
    - [BrotliUtils 类](#brotliutils)
    - [Deflator 类](#deflator-类)
    - [Inflator 类](#inflator-类)
    - [ZipArchive 类](#ziparchive-类)
- [缓冲池 (命名空间 bufferpool) 实验阶段](#缓冲池-namespace-bufferpool-实验阶段)
- [使用示例](#使用示例)

## 基础流 (namespace base)

---

### IStream 接口

流操作基础接口，所有流类型都实现此接口

**属性：**

- `canRead`: boolean - 是否可读
- `canWrite`: boolean - 是否可写
- `canSeek`: boolean - 是否支持随机访问
- `position`: number - 当前指针位置
- `length`: number - 流长度（通常用于截断文件）

**方法：**

- `copyTo(stream: IStream, bufferSize?: number): void`
- `copyToAsync(stream: IStream, bufferSize?: number): Promise<void>`
- `seek(offset: number, origin: SeekOrigin): void`
- `read(buffer: BufferLike, offset?: number, count?: number): number`
- `readAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>`
- `write(buffer: BufferLike, offset?: number, count?: number): number`
- `writeAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>`
- `flush(): void`
- `flushAsync(): Promise<void>`
- `close(): void`
- `closeAsync(): Promise<void>`

### BufferLike 类型

```typescript
type BufferLike = ArrayBufferLike | Uint8Array
```

### SeekOrigin 枚举

```typescript
enum SeekOrigin {
Begin, // 流开始位置
Current, // 当前位置
End // 流末端
}
```

### FileMode 枚举

```typescript
enum FileMode {
READ = 0x00, // 只读
WRITE = 0x01, // 只写
APPEND = 0x02, // 追加模式
TRUNC = 0x04, // 截断模式
CREATE = 0x08 // 文件不存在时创建
}
```

### ToArrayBufferOptions

```typescript
interface ToArrayBufferOptions {
offset?: number;
length?: number;
}
```

### FileStream 类

文件流实现，继承自 IStream

**构造函数：**

- `new FileStream(path: string, mode? : FileMode)` 通过路径打开文件流
- `new FileStream(fd: number, mode? : FileMode)` 通过文件标识打开文件流
- `new FileStream(rawFile: resourceManager.RawFileDescriptor)` 通过rawfile描述符打开文件流，此模式为只读

### MemoryStream 类

内存流实现，继承自 IStream

**构造函数：**

- `new MemoryStream(capacity?: number)` 指定初始容量创建内存流
- `new MemoryStream(buffer: BufferLike)` 创建内存流并将缓冲数据写入

**特有方法：**

- `toArrayBuffer(options?: ToArrayBufferOptions): ArrayBuffer`  返回内存流数据（不修改指针位置）

### MemfdStream 类

内存流实现，继承自 IStream,此版本用于创建基于内存的文件描述符，使用场景比如文件数据在内存，使用image工具创建imageSource时传入fd用于解码;使用官方rcp发送数据时，通过fd创建fs.Stream等。
**
*通常情况下，从压缩流解密出原始文件或从网络接收文件数据，需要保存到本地后再通过官方提供的读取工具使用。这种流程下，数据要经过解码/网络等待->
写入文件->读取***

**构造函数：**

- `new MemfdStream(buffer?: BufferLike)` 指定缓冲初始化内存流

**特有方法：**

- `toArrayBuffer(options?: ToArrayBufferOptions): ArrayBuffer`  返回内存流数据（不修改指针位置）
- `sendFile(fd: number, options?: SendFileOptions): boolean` 将数据通过文件描述符发送到文件
- `sendFile(path: string, mode: number, options?: SendFileOptions): boolean` 将数据发送到指定的文件,打开模式，使用官方的fileIO.openMode
- `sendFileAsync(fd: number, options?: SendFileOptions): Promise<boolean>` sendFile异步方法
- `sendFileAsync(path: string, mode: number, options?: SendFileOptions): Promise<boolean>` sendFile异步方法

**特有属性：**

- `get fd(): number`  获取文件描述符fd

***SendFileOptions***

```typescript
interface SendFileOptions {
offset?: number //MemfdStream指针偏移， 默认0
length?: number //发送数据长度，默认内存流大小减去offset
autoClose?: boolean //传入fd时生效，发送完数据是否自动关闭fd
}
```

### createFSStream 方法

- `function createFSStream(stream: IStream): fileIo.Stream` 将 IStream 转换为官方文件流

### createStreamChunk 方法

- `function createStreamChunk(stream: IStream, chunkSize?: number): ChunkIterator` 将可读流创建分块迭代器，用于数据流迭代操作
- `chunkSize` 默认大小8kb

## 流工具 (namespace streamUtils)

---

### streamToReadable

`function streamToReadable(stream: base.IStream): stream.Readable` 将可写流转换成stream.Writable

### streamToWriteable

`function streamToWriteable(stream: base.IStream): stream.Writable`  将可写流转换成stream.Writable

### createMultiWritable

`function createMultiWritable(...stream: base.IStream[]): stream.Writable` 用于多路可写流，将可读流同时写入多个可写流

## 压缩流 (namespace compression)

---

### DeflateStream 类

DEFLATE 压缩/解压缩流

**构造函数：**

- `new DeflateStream(stream:base.IStream,mode:DeflateStreamMode, option ? : DeflateStreamOption)`

**选项参数：**

```typescript
interface DeflateStreamOption {
leaveOpen?: boolean; // 是否保持底层流打开
windowBits?: number; // 窗口大小（默认-15）
bufferSize?: number; // 缓冲区大小
compressionLevel?: number // 压缩等级
}
```

### Deflator 类

DEFLATE 压缩工具

**静态方法：**

- `static deflate(chunk: BufferLike, option?: DeflatorOption): Uint8Array` 压缩缓冲区数据
- `static createStream(option ? : DeflatorOption): DeflatorStream` 创建官方stream.duplex转换流

**实例方法：**

- `push(chunk: BufferLike, end ? : boolean):void` 将数据写入Deflator，end为true时结束
- `result(): Uint8Array` 获取压缩结果
- `reset(): void` 重置Deflator
- `dispose(): void` 释放对象

### Inflator 类

DEFLATE 解压缩工具

**静态方法：**

- `static inflate(chunk: BufferLike):Uint8Array` 解压缓冲区数据
- `static createStream(): InflatorStream` 创建官方stream.duplex转换流

**实例方法：**

- `push(chunk:BufferLike, end ? : boolean):void` 将数据写入Inflator，end为true时结束
- `result():Uint8Array` 获取解压结果
- `reset():void` 重置Inflator
- `dispose(): void` 释放对象

### ZipArchive 类

ZIP 压缩包操作

**构造函数：**

- `new ZipArchive(stream: base.IStream, option ? : ZipArchiveOption)`
- `new ZipArchive(path:string, option ? : ZipArchiveOption)`
- `new ZipArchive(rawFile: resourceManager.RawFileDescriptor, password ? : string)`

**主要方法：**

- `get entries(): ZipArchiveEntry[]`
- `createEntry(entryName: string, compressionLevel ? : number):ZipArchiveEntry` 在非Read模式下可使用
- `close():void`

**ZipArchiveEntry 方法：**

- `open(): base.IStream`
- `delete ():void`

**ZipArchiveEntry 属性：**

- `get uncompressedSize(): number` 仅在Read模式或未调用open的Update模式能获取到正确值，否则返回0
- `get compressedSize(): number` 仅在Read模式或未调用open的Update模式能获取到正确值，否则返回0
- `get isOpened(): boolean`
- `get fullName(): string`
- `set fullName(value: string)`
- `get isEncrypted(): boolean`
- `get compressionLevel(): number`
- `get fileComment(): string`
- `get lastModifier(): Date`
- `get crc32(): number`

## BrotliStream

***Brotli压缩/解压流，继承IStream所有方法***

**构造函数**

- `new BrotliStream(stream:base.IStream,mode:BrotliStreamMode, option ? : BrotliStreamOptions)`

**参数说明**

***BrotliStreamMode***

```typescript
enum BrotliStreamMode {
Compress,
Decompress
}
```

***BrotliStreamOptions***

```typescript
interface BrotliStreamOptions {
quality?: number; //压缩等级
lgWin?: number; //压缩窗口
mode?: number; //压缩模式
leaveOpen?: boolean; //关闭流时，是否保持构造函数中的stream是否保持打开
bufferSize?: number; //缓冲区大小 默认8k
}
```

## BrotliUtils

Brotli工具类

**方法**

- `function compress(buffer: BufferLike | string, config?: BrotliConfig): ArrayBuffer | undefined`

- `function decompress(buffer: BufferLike | string): ArrayBuffer | undefined`

- `function compressAsync(buffer: BufferLike | string, config?: BrotliConfig): Promise<ArrayBuffer>`

- `function decompressAsync(buffer: BufferLike | string): Promise<ArrayBuffer>`

## 缓冲池 (namespace bufferpool) 实验阶段

---

### BufferPool 抽象类

```typescript
abstract class BufferPool {
  abstract acquire(size: number): ArrayBuffer

  abstract release(buffer: ArrayBuffer): void

  get stats(): BufferPoolStats

  protected updateStats(acquiring: boolean): void
}
```

***后续将会改变***

```typescript
abstract class BufferPool<TStats extends BufferPoolStats> { //通过泛型用于缓冲池返回更多信息，如命中率使用率等
  abstract acquire(size?: number): ArrayBuffer | undefined //size改为可选，用于FixedBufferPool/RingBufferPool支持, 同步接口在创建失败时返回undefined

  abstract acquireAsync(size?: number): Promise<ArrayBuffer> //异步接口用于长时间等待缓冲

  abstract release(buffer: ArrayBuffer): void

  get stats(): TStats

  protected updateStats(state: BufferState, size: number): void //用于记录创建、归还、复用、淘汰等信息
}
```

***BufferPoolStats***

```typescript
interface BufferPoolStats {
total: number
used: number
}
```

- `acquire(size?: number): ArrayBuffer` 申请size大小的ArrayBuffer
- `release(buffer: ArrayBuffer)` 归还创建的缓冲
- `get stats()` 获取缓冲池状态信息

### LruBufferPool(后续会改)

***带LRU淘汰策略的BufferPool***

```typescript
class LruBufferPool extends BufferPool {
  constructor(maxSize: number) //最大缓冲池大小

  acquire(size: number): ArrayBuffer;

  release(buffer: ArrayBuffer): void;
}
```

### FixedBufferPool、DynamicBufferPool、RingBufferPool待实现

## 使用示例

---

### 文件流基础操作

```typescript
// 读取文件
const fs = new base.FileStream("test.txt", base.FileMode.READ);
const buffer = new Uint8Array(1024);
const bytesRead = fs.read(buffer);
fs.close();

// 写入文件
const outFs = new base.FileStream("output.txt", base.FileMode.CREATE | base.FileMode.WRITE);
const data = new TextEncoder().encode("Hello World");
outFs.write(data);
outFs.close();
```

### 使用迭代器读取流

```typescript
const fs = new FileStream('1.txt')
const chunks = base.createStreamChunk(fs)
for await (let buffer of chunks) {
}
for (let buffer of chunks) {
}
```

### 内存流操作

```typescript
const ms = new base.MemoryStream();
ms.write(new TextEncoder().encode("Memory Stream Test"));
ms.seek(0, base.SeekOrigin.Begin);
const readBuffer = new Uint8Array(20);
ms.read(readBuffer);
console.log(new TextDecoder().decode(readBuffer));
```

### 使用 DEFLATE 压缩

```typescript
// 压缩文件
const source = new base.FileStream("source.txt", base.FileMode.READ);
const compressed = new base.FileStream("compressed.deflate", base.FileMode.CREATE);
const deflateStream = new compression.DeflateStream(
  compressed,
  compression.DeflateStreamMode.Compress,
  { compressionLevel: compression.CompressionLevel.BEST_COMPRESSION }
);
source.copyTo(deflateStream);
deflateStream.close();
source.close();

// 解压文件
const decompressed = new base.FileStream("decompressed.txt", base.FileMode.CREATE);
const inflateStream = new compression.DeflateStream(
  new base.FileStream("compressed.deflate", base.FileMode.READ),
  compression.DeflateStreamMode.Decompress
);
inflateStream.copyTo(decompressed);
inflateStream.close();
decompressed.close();
```

### ZIP 压缩包操作

```typescript
// 创建 ZIP 文件
const zip = new compression.ZipArchive("test.zip", { mode: ZipArchiveMode.Create });
const entry = zip.createEntry("data.txt");
const stream = entry.open();
stream.write(new TextEncoder().encode("ZIP File Content"));
stream.close();
zip.close();

// 读取 ZIP 文件
const zipReader = new compression.ZipArchive("test.zip");
for (const entry of zipReader.entries) {
  const stream = entry.open();
  const buffer = new Uint8Array(entry.uncompressedSize);
  stream.read(buffer);
  console.log(`File: ${entry.fullName}, Content: ${new TextDecoder().decode(buffer)}`);
  stream.close();
}
zipReader.close();

```

### Deflator/Inflator

```typescript
//let buffer = new Uint8Array(1000);
//deflate压缩数据
let result = compression.Deflator.deflate(buffer);
//inflate解压数据
result = compression.Inflator.inflate(result);

//参考pako
let deflator = new compression.Deflator();
deflator.push(buffer, true);
result = deflator.result();

let inflator = new compression.Inflator();
inflator.push(buffer, ture);
result = inflator.result();

//创建stream.Transform转换流

let deflateTransform = Deflator.createStream();
let inflateTransform = Inflator.createStream();

//rs为可读流，ws为可写流
//ps: 鸿蒙好像没提供pipeline方法  
rs.pipe(deflateTransform);
deflateTransform.pipe(ws);
```

### 将IStream转换成官方fs.Stream

```typescript
base.createFSStream(new base.MemoryStream());
```

### 将数据拷贝到ArrayBuffer

#### 方式1 - 通过read方法分段读取

```typescript
const bufferSize = 10000; //假设已知要接受10000字节数据
let buffer = new ArrayBuffer(bufferSize);
const fs = new base.FileStream("1.txt");
let actualRead = 0;
let totalRead = 0;
while (totalRead < bufferSize) {
  actualRead = fs.read(buffer, totalRead, bufferSize - totalRead);
  totalRead += actualRead;
}
```

#### 方式2 - 通过copyTo方法直接读取

方式2相比方式1会快上至少一倍

```typescript
const ms = new base.MemoryStream(10000); //提前预设容量减少扩容次数
const fs = new base.FileStream("1.txt");
fs.copyTo(ms);
const buffer = ms.toArrayBuffer();
```

### gzip流加解密

***GZip流使用DeflateStream设置windowBits为31实现***

```typescript
const outputStream = new base.MemoryStream();
const inputStream = new base.MemoryStream();
const gzipCompressStream =
  new compression.DeflateStream(outputStream, compression.DeflateStreamMode.Compress, { windowBits: 31 });
const gzipDecompressStream =
  new compression.DeflateStream(inputStream, compression.DeflateStreamMode.Decompress, { windowBits: 31 });
```

### MemfdStream使用示例

```typescript
import { fileIO as fs } from "@kit.CoreFileKit";

const zip = new compression.ZipArchive("test.zip");
const entry = zip.getEntry('test.jpg');
const stream = entry.open();
const ms = new base.MemfdStream();
stream.copyTo(ms);
const imageSource = image.createImageSource(ms.fd); //通过MemfdStream内部fd创建image实例
const pixelMap = imageSource.createPixelMapSync();
ms.sendFile(getContext(this).cacheDir + '/test.jpg', fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY); //直接缓存文件，下次读取时从缓存目录读取
ms.close();
```

### Brotli使用示例

```typescript
import { compression, base } from '@jemoc/stream'

const ms = new base.MemoryStream();
const bs = new compression.BrotliStream(ms, compression.BrotliStreamMode.Compress); //使用方法与DeflateStream类似，不多做介绍

const compressData = compression.BrotliUtils.compress("hello world"); //使用工具类进行brotli压缩
const decompressData = compression.BrotliUtils.decompress(compressData); //使用工具类进行brotli解压

```

## 如果使用遇到问题

---

### 使用过程中发现任何问题都可以提 [Issue](https://gitee.com/jiemoccc/jemoc-memory/issues)