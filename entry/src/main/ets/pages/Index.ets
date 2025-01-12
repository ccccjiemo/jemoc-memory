import { DeflateStream, FileStream, MemoryStream, using, ZipArchive, ZipCryptoStream } from '@jemoc/stream';
import { fileUri } from '@kit.CoreFileKit';

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
          getContext(this).resourceManager.getRawFileContent('test.zip', (err, data) => {
            let ms = new MemoryStream(data.length)
            ms.write(data);
            let archive = new ZipArchive(ms, { password: '12345678' });

            let entries = archive.entries;
            let stream = entries[0].open();
            let dir = getContext(this).cacheDir + '/test.pdf';

            let fs = new FileStream(dir, 0x19);
            let bytesRead = 0;
            let buffer = new ArrayBuffer(4096);
            let start = Date.now();
            while ((bytesRead = stream.read(buffer)) != 0) {
              fs.write(buffer, 0, bytesRead);
            }
            fs.close();
            stream.close();
            console.log('1');
            console.log(`decompress: ${Date.now() - start}ms`)
            //
            // let total = 92961877;
            // let buffer = new ArrayBuffer(8196);
            // let ms = new MemoryStream(data.length)
            // ms.write(data);
            // ms.seek(0x55, 0);
            // let crypto = new ZipCryptoStream(ms, 0, "12345678", 0x4b637667);
            // let ds = new DeflateStream(crypto, 1, {bufferSize: 8192});
            // let fs = new FileStream(dir, 0x19);
            // let start = Date.now();
            // while (total != 0) {
            //   let readBytes = ds.read(buffer, 0, Math.min(buffer.byteLength, total));
            //   let writeBytes = fs.write(buffer, 0, readBytes);
            //   total -= readBytes;
            // }
            // fs.close();
            // crypto.close();
            // ms.close();
            // console.log(`decompress: ${Date.now() - start}ms`)

          })
        })
    }
    .height('100%')
    .width('100%')
  }
}