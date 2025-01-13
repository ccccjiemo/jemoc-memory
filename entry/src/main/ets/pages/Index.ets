import { DeflateStream, FileStream, IStream, MemoryStream, using, ZipArchive, ZipCryptoStream } from '@jemoc/stream';
import { fileUri } from '@kit.CoreFileKit';
import { util } from '@kit.ArkTS';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    RelativeContainer() {
      Text(this.message)
        .id('HelloWorld')
        .fontSize(50)
        .fontWeight(FontWeight.Bold)
        .alignRules({
          center: { anchor: '__container__', align: VerticalAlign.Center },
          middle: { anchor: '__container__', align: HorizontalAlign.Center }
        })
      Button('test')
        .onClick(() => {
          // getContext(this).resourceManager.getRawFileContent('test.zip', (err, data) => {
          //   let ms = new MemoryStream(data.length)
          //   ms.write(data);
          //   let archive = new ZipArchive(ms, { password: '12345678' });
          //
          //   let entries = archive.entries;
          //   let stream = entries[0].open();
          //   let dir = getContext(this).cacheDir + '/test.pdf';
          //
          //   let fs = new FileStream(dir, 0x19);
          //   let bytesRead = 0;
          //   let buffer = new ArrayBuffer(4096);
          //   let start = Date.now();
          //   while ((bytesRead = stream.read(buffer)) != 0) {
          //     fs.write(buffer, 0, bytesRead);
          //   }
          //   fs.close();
          //   stream.close();
          //   console.log('1');
          //   console.log(`decompress: ${Date.now() - start}ms`)
          // });

          // getContext(this).resourceManager.getRawFileContent('0002.jpg', (err, data) => {
          //   let dir = getContext(this).cacheDir + '/test.zip';
          //   let fs = new FileStream(dir, 0x09);
          //   let archive = new ZipArchive(fs, { mode: 2, password: '12345678' });
          //   let entry = archive.createEntry("0002.jpg");
          //   let stream = entry.open();
          //   stream.write(data);
          //   stream.close();
          //   archive.close();
          // });

          getContext(this).resourceManager.getRawFileContent('test (2).zip', (err, data) => {
            let ms = new MemoryStream();
            ms.write(data);
            let archive = new ZipArchive(ms, { mode: 1, leaveOpen: true });
            let entry = archive.entries[0];
            let stream: IStream = entry.open();
            let buffer = util.TextEncoder.create('utf-8').encodeInto("update test;test chinese 中文测试; ");
            stream.write(buffer);
            stream.close();
            archive.close();

            console.log('finish');
          })
        })
    }
    .height('100%')
    .width('100%')
  }
}