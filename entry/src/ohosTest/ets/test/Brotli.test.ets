import { describe, expect, it } from "@ohos/hypium";
import * as brotli from 'brotli-js'

function generateRandomData(size: number): Uint8Array {
  const data = new Uint8Array(size);
  for (let i = 0; i < size; i++) {
    data[i] = Math.floor(Math.random() * 256);
  }
  return data;
}

interface CompressOptions {
  quality?: number;
  mode?: number;
  lgwin?: number;
}

export default function brotliTest() {
  describe('BrotliPerformanceTests', () => {
    // 数据大小列表（单位：字节）
    const dataSizes = [
      1024,          // 1 KB
      10 * 1024,     // 10 KB
      100 * 1024,    // 100 KB
      1024 * 1024,   // 1 MB
      10 * 1024 * 1024 // 10 MB
    ];

    it('performance', 0,() => {
      for (const size of dataSizes) {
        console.log(`--- Testing with data size: ${size} bytes ---`);

        // 生成随机数据
        const originalData = generateRandomData(size);

        // 压缩选项
        const options:CompressOptions = {
          mode: 0, // 0 = generic, 1 = text, 2 = font (WOFF2)
          quality: 11, // 压缩质量，范围为 0-11
          lgwin: 22 // 窗口大小，范围为 10-24
        };

        // 压缩性能测试
        const compressStart = Date.now();
        const compressedData: string | null = brotli.compress(originalData, options);
        const compressEnd = Date.now();

        console.log(`Compression time for ${size} bytes: ${(compressEnd - compressStart).toFixed(2)} ms`);
        console.log(`Compressed size: ${compressedData?.length} bytes`);

        // 解压缩性能测试
        const decompressStart = Date.now();
        const decompressedData : string | null= brotli.decompress(compressedData);
        const decompressEnd = Date.now();

        console.log(`Decompression time for ${size} bytes: ${(decompressEnd - decompressStart).toFixed(2)} ms`);
        console.log(`Decompressed size: ${decompressedData?.length} bytes`);

        // 验证数据完整性
        expect(decompressedData!).assertEqual(originalData);
      }
    });
  });
}