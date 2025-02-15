import { base, compression, bufferpool } from '@jemoc/stream';
import { fileIo, fileUri } from '@kit.CoreFileKit';
import { util } from '@kit.ArkTS';
import image from '@ohos.multimedia.image';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    Column() {
      Button('test fd').onClick(async (event: ClickEvent) => {
        const cache = getContext(this).cacheDir;
        const fd = await getContext(this).resourceManager.getRawFd('0002.jpg');
        const fs = new base.FileStream(fd);
        const ms = new base.MemfdStream();
        await fs.copyToAsync(ms);

        const source = image.createImageSource(ms.fd);
        const pixelMap = await source.createPixelMap();



        console.log('1')

      })
      Button('test buffer pool')
        .onClick(() => {
          const pool = new bufferpool.LruBufferPool(10);
          for (let i = 0; i < 1000; i++) {
            const buffer = pool.acquire(1024);
            pool.release(buffer);
          }
        })
      Button('test deflate')
        .onClick(() => {
          getContext(this).resourceManager.getRawFileContent('lorem_1mb.txt', (err, data) => {
            if (err) {
              return;
            }
            let start = Date.now();
            let result = compression.Deflator.deflate(data, { bufferSize: data.length });
            let end = Date.now();
            console.log(`-----------deflate---${end - start}ms`);
            start = end;
            result = compression.Inflator.inflate(result);
            end = Date.now();
            console.log(`-----------inflate---${end - start}ms`);
            let fs = new base.FileStream(getContext(this).cacheDir + '/deflate_test.txt', 0x9);
            fs.write(result);
            fs.close();
          })

        })
      Button('test ziparchive')
        .onClick(async () => {
          try {
            const context = getContext(this);
            const rawFile = context.resourceManager.getRawFdSync('妖精の尾巴 第01卷.zip');
            const fs = new base.FileStream(rawFile);
            const zip = new compression.ZipArchive(fs);
            const entry = zip.entries[1];
            const fullname = entry.fullName;
            const buffer = new ArrayBuffer(entry.uncompressedSize);
            const stream = entry.open();
            let totalRead = 0;
            let ms = new base.MemfdStream();
            let start = Date.now();
            // while (totalRead < entry.uncompressedSize) {
            //   let actualRead = await stream.readAsync(buffer, totalRead, entry.uncompressedSize - totalRead);
            //   totalRead += actualRead;
            // }
            await stream.copyToAsync(ms);
            let b1 = ms.toArrayBuffer();
            let end = Date.now();

            console.log(`${end - start}`);
            //await stream.closeAsync();
            const file =
              await fileIo.open(context.cacheDir + '/0002.jpg', fileIo.OpenMode.CREATE | fileIo.OpenMode.WRITE_ONLY);
            await fileIo.write(file.fd, b1);
            await fileIo.close(file);
            ms.close();
          } catch (e) {
            console.log(e);
          }

        })
      Button('test')
        .onClick(async () => {
          const rawFd = await getContext(this).resourceManager.getRawFd('test(crypto).zip');
          // const fs = new base.FileStream(rawFd);
          //
          // const length = fs.length;
          // const buffer = new ArrayBuffer(length);
          // fs.read(buffer);

          const zip = new compression.ZipArchive(rawFd, '12345678');
          const entry = zip.entries[0];
          const stream = entry.open();
          const ms = new base.MemoryStream();
          stream.copyTo(ms);
          const file = ms.toArrayBuffer();
          const result = util.TextDecoder.create('utf-8').decodeToString(new Uint8Array(file));
          console.log('1');
          // getContext(this).resourceManager.getRawFileContent('test(crypto).zip', (err, data) => {
          //   let ms = new base.MemoryStream();
          //   ms.write(data);
          //   let archive = new compression.ZipArchive(ms, { mode: 1, leaveOpen: true, password: '12345678' });
          //   let entry = archive.entries[0];
          //   let buffer = util.TextEncoder.create('utf-8').encodeInto("update test;test chinese 中文测试; ");
          //   entry.delete();
          //   entry = archive.createEntry("t/test++.txt");
          //   entry.isEncrypted = true;
          //   let stream = entry.open();
          //   stream.write(buffer);
          //   stream.close();
          //   archive.close();
          //   let fs = new base.FileStream(getContext(this).cacheDir + '/test2.zip', 0x9);
          //   fs.length = 0;
          //   ms.seek(0, 0);
          //   ms.copyTo(fs);
          //   fs.close();
          //   console.log('finish');
          // })
        })
    }
    .height('100%')
    .width('100%')
  }
}

