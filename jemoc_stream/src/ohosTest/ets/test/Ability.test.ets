import { hilog } from '@kit.PerformanceAnalysisKit';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { DeflateStream, MemoryStream } from 'libjemoc_stream.so';
import { util } from '@kit.ArkTS';

export default function abilityTest() {
  describe('ActsAbilityTest', () => {
    // Defines a test suite. Two parameters are supported: test suite name and test suite function.
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
      hilog.info(0x0000, 'testTag', '%{public}s', 'it begin');
      let a = 'abc';
      let b = 'b';
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      expect(a).assertContain(b);
      expect(a).assertEqual(a);

      let ms = new MemoryStream(256);

      let result = await ms.writeAsync(new Uint8Array([1, 2, 3, 4, 5]));

      expect(ms.canSeek).assertEqual(true);
      expect(ms.canRead).assertEqual(true);
      expect(ms.canWrite).assertEqual(true);
      expect(ms.length).assertEqual(5);
      expect(ms.capacity).assertEqual(256);
      expect(ms.position).assertEqual(5);

      ms.write(new Uint8Array([1, 2, 3, 4, 5]));
      expect(ms.position).assertEqual(5);
      ms.seek(0, 0);
      let arr = new ArrayBuffer(5);
      ms.read(arr);

      ms.close();
      try {
        expect(ms.write(arr));
      } catch (e) {
        console.log(e);
      }


      try {
        let test_buff = util.TextEncoder.create('utf-8').encodeInto('Deflate测试abcdefghijklmn');
        let cms = new MemoryStream(100);
        let compressStream = new DeflateStream(cms, 0, {
          leaveOpen: true
        });
        compressStream.write(test_buff);
        compressStream.close();
        let dcms = new MemoryStream(100);
        cms.seek(0, 0);
        let decompressStream = new DeflateStream(cms, 1);
        let readBytes = 0;
        let cache = new ArrayBuffer(20);

        try {
          while ((readBytes = decompressStream.read(cache)) != 0) {
            dcms.write(cache, 0, readBytes);
          }
        } catch (e) {
          console.log(e);
        }

        decompressStream.close();
        let decompress_result = util.TextDecoder.create('utf-8').decodeToString(new Uint8Array(dcms.toArrayBuffer()));
        dcms.close();
        console.log(decompress_result);
      } catch (e) {
        console.log(e);
      }


    })
  })
}