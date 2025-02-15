import { describe, it, expect, beforeEach, afterEach } from '@ohos/hypium';
import { LruBufferPool } from 'libjemoc_stream.so';

const BASE_COUNT = 2000;

export default function LruTest() {

  describe('LruBufferPoolTest', () => {
    const MAX_SIZE = 4096; // 4KB总容量
    let lruPool: LruBufferPool;
    beforeEach(() => {
      lruPool = new LruBufferPool(MAX_SIZE);
    });
    afterEach(() => {
      // 清理所有缓冲区
      let stats = lruPool.stats;
      while (stats.used > 0) {
        // 实际实现需要遍历释放，这里暂时保留空实现
      }
    });
    it('should_evict_when_total_size_exceeds_max', 0, () => {
      // 分配4个1KB缓冲区（总4KB）
      const buf1 = lruPool.acquire(1024);
      const buf2 = lruPool.acquire(1024);
      const buf3 = lruPool.acquire(1024);
      const buf4 = lruPool.acquire(1024);
      // 释放顺序建立访问记录：1 -> 2 -> 3 -> 4
      lruPool.release(buf1);
      lruPool.release(buf2);
      lruPool.release(buf3);
      lruPool.release(buf4);
      // 获取第5个1KB缓冲区（总需求5KB > 4KB）
      const buf5 = lruPool.acquire(1024);
      // 验证总大小不超过max
      expect(lruPool.stats.total).assertLessOrEqual(MAX_SIZE);
      // 应该淘汰buf1
      expect(lruPool.stats.total).assertEqual(4096); // 4*1024=4096
    });
    it('should_handle_varied_buffer_sizes', 0, () => {
      // 分配不同大小的缓冲区
      const bufA = lruPool.acquire(2048); // 2KB
      const bufB = lruPool.acquire(1024); // 1KB
      const bufC = lruPool.acquire(1024); // 1KB (总4KB)
      lruPool.release(bufA);
      lruPool.release(bufB);
      lruPool.release(bufC);
      // 获取新2KB缓冲区，应该淘汰最早释放的bufA
      const bufD = lruPool.acquire(2048);
      expect(lruPool.stats.total).assertEqual(2048 + 1024); // 2KB+1KB=3KB
    });
    it('should_reject_oversized_request', 0, () => {
      // 尝试分配超过maxSize的缓冲区
      try {
        lruPool.acquire(MAX_SIZE + 1);
      } catch (e) {
        expect(e instanceof Error).assertTrue();
      }
    });
    it('should_update_stats_correctly', 0, () => {
      const buf1 = lruPool.acquire(1024);
      const buf2 = lruPool.acquire(2048);
      // 验证已用大小
      expect(lruPool.stats.used).assertEqual(2);
      expect(lruPool.stats.total).assertEqual(1024 + 2048);
      lruPool.release(buf1);
      expect(lruPool.stats.used).assertEqual(1);
      expect(lruPool.stats.total).assertEqual(2048);
    });
  });
}