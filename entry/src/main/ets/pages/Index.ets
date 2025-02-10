import { base, compression } from '@jemoc/stream';
import { fileIo, fileUri } from '@kit.CoreFileKit';
import { util } from '@kit.ArkTS';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    Column() {
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

