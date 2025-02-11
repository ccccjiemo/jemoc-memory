## @jemoc/stream

---
方法参考.net Stream。zlib-ng提供Inflate、Deflate支持。

实现MemoryStream，自动扩容内存流

实现FileStream,适应该库方法

实现DeflateStream完成deflate数据压缩解压缩流

实现ZipArchive完成zip的压缩和解压

Deflator、Inflator可以直接进行deflate压缩和inflate解压和创建鸿蒙stream.Transform转换流

gzip加解压缩通过设置windowBits实现。所有方法windowBits默认-15
___

### 如何安装

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
    - [createFSStream 函数](#createfsstream-函数)
- [压缩流 (命名空间 compression)](#压缩流-namespace-compression)
    - [DeflateStream 类](#deflatestream-类)
    - [Deflator 类](#deflator-类)
    - [Inflator 类](#inflator-类)
    - [ZipArchive 类](#ziparchive-类)
- [使用示例](#使用示例)

---

## 基础流 (namespace base)

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

- `toArrayBuffer(): ArrayBuffer`  返回内存流数据（不修改指针位置）

### MemfdStream 类

内存流实现，继承自 IStream,此版本用于创建基于内存的文件描述符，使用场景比如文件数据在内存，使用image工具创建imageSource时传入fd用于解码;使用官方rcp发送数据时，通过fd创建fs.Stream等。

**构造函数：**

- `new MemfdStream(buffer?: BufferLike)` 指定缓冲初始化内存流

**特有方法：**

- `toArrayBuffer(): ArrayBuffer`  返回内存流数据（不修改指针位置）

**特有属性：**

- `get fd(): number`  获取文件描述符fd

### createFSStream 函数

- `function createFSStream(stream: IStream): fileIo.Stream` 将 IStream 转换为官方文件流

---

## 压缩流 (namespace compression)

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

---

## 使用示例

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

## 如果使用遇到问题

---

### 使用过程中发现任何问题都可以提 [Issue](https://gitee.com/jiemoccc/jemoc-memory/issues)