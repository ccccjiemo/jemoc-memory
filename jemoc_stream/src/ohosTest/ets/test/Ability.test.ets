import { hilog } from '@kit.PerformanceAnalysisKit';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { DeflateStream, MemoryStream, ZipCryptoStream } from 'libjemoc_stream.so';
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
      let ms = new MemoryStream(256);
      let buffer = new ArrayBuffer(10);
      ms.write(new Uint8Array([0x66, 0x56, 0x9b, 0x09, 0xcd, 0x58, 0xf8, 0xc4, 0x16, 0x3d, 0x66, 0xd2, 0x00, 0xc4, 0xc4,
        0x83]));
      ms.seek(0, 0);
      let crypto = new ZipCryptoStream(ms, 0, "12345678", 0x4b637667);
      let readBytes = crypto.read(buffer);

      console.log('1');
    })
  })
}