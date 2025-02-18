## [1.1.2] - 2025-02-18

- IStream的异步方法增加锁机制，防止读写错误
- 新增BrotliStream、BrotliUtils，支持Brotli加解密
- toArrayBuffer方法增加范围设置

---

## [1.1.1] - 2025-02-16

- 修复MemfdStream的sendFile问题
- MemfdStream增加sendFileAsync方法,新增传入文件路径支持

---

## [1.1.0] - 2025-02-15

- 修复MemfdStream创建的fd无法用于image.createImageSource问题
- MemfdStream新增sendFile方法，用于将内存数据保存到文件
- 新增streamUtils工具集，用于库提供与@ohos.stream转换
- 新增实验性BufferPool
- base命名空间下新增分块迭代器工具

## [1.0.5] - 2025-02-11

- 新增MemfdStream，用于需要fd操作的场景

## [1.0.4] - 2025-02-11

- 修复ZipArchiveEntry的fullName编码是unicode时候乱码的错误
- 更正声明文件中MemoryStream的构造函数声明

---

## [1.0.3] - 2025-02-10

- 修复ZipArchive在ZipCrypto加密无法解压问题
- FileStream增加RawFile支持(仅支持读取)
- ZipArchive增加RawFile支持(仅支持Read模式)
- 让DeepSeek帮我重写README.md

---

## [1.0.2] - 2025-02-09

- 修复声明文件中MemoryStream缺失toArrayBuffer方法
- 修复Native中未导出ZipArchiveEntry的isDeleted方法
- ZipArchiveEntry增加compressedSize和unCompressedSize获取方法(仅在Read模式或者Update模式\[未使用Open\])
- base命名空间下增加createFSStream方法，用于将base.IStream转换成fs.Stream使用(没有什么是增加中间层不能解决的)

---

## [1.0.1] - 2025-01-23

- 修复JS ZipArchiveEntry移除错误
- 修改MemoryStream缓存策略，capacity扩容执行4k对齐

---

## [1.0.0] - 2025-1-16

- 发布初版

