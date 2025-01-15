import { base, compression } from '@jemoc/stream';
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
      Button('test deflate')
        .onClick(() => {
          getContext(this).resourceManager.getRawFileContent('lorem_1mb.txt', (err, data) => {
            if (err) {
              return;
            }
            let start = Date.now();
            let result = compression.Deflator.deflate(data);
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
        .onClick(() => {
          getContext(this).resourceManager.getRawFileContent('test(crypto).zip', (err, data) => {
            let ms = new base.MemoryStream();
            ms.write(data);
            let archive = new compression.ZipArchive(ms, { mode: 1, leaveOpen: true, password: '12345678' });
            let entry = archive.entries[0];
            let buffer = util.TextEncoder.create('utf-8').encodeInto("update test;test chinese 中文测试; ");
            entry.delete();
            entry = archive.createEntry("test++.txt");
            entry.isEncrypted = true;
            let stream = entry.open();
            stream.write(buffer);
            stream.close();
            archive.close();
            let fs = new base.FileStream(getContext(this).cacheDir + '/test2.zip', 0x9);
            fs.length = 0;
            ms.seek(0, 0);
            ms.copyTo(fs);
            fs.close();
            console.log('finish');
          })
        })
    }
    .height('100%')
    .width('100%')
  }
}