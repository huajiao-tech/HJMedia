import { NavStackUtil } from '../util/NavStackUitl'
import { ToastUtils } from '../util/ToastUtils';

@Builder
export function PageBuilder() {
  IndexV2()
}

@Entry
@ComponentV2
struct IndexV2 {
  navStack:NavPathStack = NavStackUtil.getNavStack();
  routeNames: string[] = ["PlayerPage", "PusherPage"]

  aboutToAppear(): void {
    ToastUtils.init(this.getUIContext())
  }

  build() {
    Navigation(this.navStack){
      Column() {
        Repeat(this.routeNames)
          .each((routeName) => {
            Button(routeName.item, { stateEffect: true, type: ButtonType.Capsule })
              .width('80%')
              .height(40)
              .margin(20)
              .onClick(() => {
                NavStackUtil.jumpPush({name: routeName.item})
              })
          })
      }.width('100%').height('100%')
    }
    .title("HJMedia Example")
    .mode(NavigationMode.Stack)
  }
}