import hilog from '@ohos.hilog';
import testNapi from 'libentry.so'

@Entry
@Component
struct Index {
  @State message: string = 'Hello World'

  build() {
    Row() {
      Column() {
        Text(this.message)
          .fontSize(50)
          .fontWeight(FontWeight.Bold)
          .onClick(() => {
            hilog.info(0x0000, 'testTag', 'Test NAPI 2 + 3 = %{public}d', testNapi.add(2, 3));

            testNapi.RegSvcRsp((s:string)=>  {
              hilog.info(0x0000, 'testTag', 'Test NAPI =%{public}s', s);
            });

          })
      }
      .width('100%')
    }
    .height('100%')
  }
}
