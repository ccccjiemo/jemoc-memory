import { hilog } from '@kit.PerformanceAnalysisKit';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import * as brotli from 'brotli-js'
import { util } from '@kit.ArkTS';
import { base, compression } from '@jemoc/stream'
import { lorem } from './lorem'

function generateRandomData(size: number): string {
  return lorem.substring(0, size);
}

interface CompressOptions {
  quality?: number;
  mode?: number;
  lgwin?: number;
}

export default function abilityTest() {
  describe('ActsAbilityTest', () => {
    // Defines a test suite. Two parameters are supported: test suite name and test suite function.

    // 数据大小列表（单位：字节）
    const dataSizes = [
      1024, // 1 KB
      10 * 1024, // 10 KB
      100 * 1024, // 100 KB
      1024 * 1024, // 1 MB
      10 * 1024 * 1024, // 10 MB
      100 * 1024 * 1024, //100MB
      1000 * 1024 * 1024,//1000MB
    ];
    beforeAll(() => {
      // Presets an action, which is performed only once before all test cases of the test suite start.
      // This API supports only one parameter: preset action function.
    })
    beforeEach(() => {
      // Presets an action, which is performed before each unit test case starts.
      // The number of execution times is the same as the number of test cases defined by **it**.
      // This API supports only one parameter: preset action function.
    })
    afterEach(() => {
      // Presets a clear action, which is performed after each unit test case ends.
      // The number of execution times is the same as the number of test cases defined by **it**.
      // This API supports only one parameter: clear action function.
    })
    afterAll(() => {
      // Presets a clear action, which is performed after all test cases of the test suite end.
      // This API supports only one parameter: clear action function.
    })
    it('assertContain', 0, async () => {
      // Defines a test case. This API supports three parameters: test case name, filter parameter, and test case function.
      // hilog.info(0x0000, 'testTag', '%{public}s', 'it begin');
      // let a = 'abc';
      // let b = 'b';
      // // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      // expect(a).assertContain(b);
      // expect(a).assertEqual(a);


    })

    it('performance', 1, () => {


      for (const size of dataSizes) {
        console.log(`--- Testing with data size: ${size} bytes ---`);

        // 生成随机数据
        const originalDataStr = generateRandomData(size);
        // 压缩选项
        const options: CompressOptions = {
          mode: 0, // 0 = generic, 1 = text, 2 = font (WOFF2)
          quality: 11, // 压缩质量，范围为 0-11
          lgwin: 22 // 窗口大小，范围为 10-24
        };


        // 压缩性能测试
        const compressStart = Date.now();
        const compressedData: ArrayBuffer | undefined = compression.BrotliUtils.compress(originalDataStr);
        const compressEnd = Date.now();

        console.log(`Compression time for ${size} bytes: ${(compressEnd - compressStart).toFixed(2)} ms`);
        console.log(`Compressed size: ${compressedData?.byteLength} bytes`);

        // 解压缩性能测试
        const decompressStart = Date.now();
        const decompressedData: ArrayBuffer | undefined = compression.BrotliUtils.decompress(compressedData!);
        const decompressEnd = Date.now();

        console.log(`Decompression time for ${size} bytes: ${(decompressEnd - decompressStart).toFixed(2)} ms`);
        console.log(`Decompressed size: ${decompressedData?.byteLength} bytes`);

        const decompressDataStr = util.TextDecoder.create().decodeToString(new Uint8Array(decompressedData))


        // 验证数据完整性
        expect(originalDataStr).assertEqual(decompressDataStr);
      }
    });
  })
}