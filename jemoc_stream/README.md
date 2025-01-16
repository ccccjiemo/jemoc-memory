## @jemoc/stream

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

---

### IStream
<details>

<summary>Interface</summary>

```typescript
 /**
 * Stream基类，所有Stream继承自IStream,不要混用同步和异步方法
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
 * @param count
 * @returns
 */
write(buffer: BufferLike, offset?: number, count?: number): number

writeAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

/**
 * 刷新流
 */
flush(): void

/**
 * 刷新流
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
```
</details>


### 如何使用

```typescript
import { base } from '@jemoc/stream'

//详情见注释
let fs = new base.FileStream(path, base.FileMode.Read);
let ms = new MemoryStream();
let ds = new DeflateStream(fs, DeflateMode.Decompress);

/**
 * 写入数据
 * buffer: Uint8Array | ArrayBuffer
 */
ms.write(buffer)
//写入10位 从buffer第10位开始
ms.write(buffer, 10, 10)

//指向流首位
ms.seek(0, base.SeekOrigin.Begin)

//读取数据上面写入的10位数据
ms.read(buffer, 0, 10)

        
//MemoryStream额外有toArrayBuffer方法
//toArrayBuffer返回丛0到流末端(length)的所有数据        
buffer = ms.toArrayBuffer()

//使用完必须释放对象
ms.close()
```

```typescript
import { compression } from '@jemoc/stream'

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

/**
 * ZipArchive参考
 */
let zip:ZipArchive
try {
  let fs = new FileStream(path, FileMode.Write)

  //参考zip使用zipcrypto加密
  let zip = new ZipArchive(fs, { mode: ZipArchiveMode.Update, leaveOpen: false, password: '12345678' })

  //获取所有entry
  let entries: ZipArchiveEntry[] = zip.entries

  //获取某个entry
  let entry: ZipArchiveEntry | undefined = zip.getEntry('123.txt')

  //假设entry存在
  let stream: base.IStream = entry.open()

  //读取全部解压数据
  let ms = new MemoryStream()
  let readBytes = 0
  let buffer = new ArrayBuffer(4096)
  while ((readBytes = stream.read(buffer)) != 0) {
    ms.write(buffer, 0, readBytes)
  }
  let result: ArrayBuffer = ms.toArrayBuffer()
  ms.close() //释放内存

  //覆写内容，update模式下可以截断数据
   stream.length = 0
   stream.write(result)
   
   stream.close() //关闭流，保存写入的数据
   


} catch (e) {
  //抛出异常
} finally {
  zip?.close()
}



```


---
LICENSE

(C) 1995-2024 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.