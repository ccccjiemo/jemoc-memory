## [1.0.3] - 2025-02-10

- 修复ZipArchive在ZipCrypto加密无法解压问题
- FileStream增加RawFile支持(仅支持读取)
- ZipArchive增加RawFile支持(仅支持Read模式)
- 让DeepSeek帮我重写README.md
-

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

---