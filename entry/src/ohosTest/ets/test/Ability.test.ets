import { Deflator } from '@jemoc/stream';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

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
      // hilog.info(0x0000, 'testTag', '%{public}s', 'it begin');
      // let a = 'abc';
      // let b = 'b';
      // // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      // expect(a).assertContain(b);
      // expect(a).assertEqual(a);

      const size = 1024 * 1024;

      let buffer = new Uint8Array(size);
      for(let i = 0; i < size; i++) {
        buffer[i] = Math.random();
      }
      let start = Date.now();

      let deflater = new Deflator();
      await deflater.pushAsync(buffer, true);
      let result = deflater.result();
      let end = Date.now();

      console.log(`----------test: ${end - start}ms`);
      console.log(`----------test: ${result.toString()}`);

    })
  })
}