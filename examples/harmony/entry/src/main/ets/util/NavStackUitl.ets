import { AppStorageV2 } from "@kit.ArkUI";

export const NAV_STACK = 'GUO_HANG_NAV_STACK'

export class NavStackUtil {
  /**
   * 获取当前页面参数
   * */
  static getParams<T>(): T {
    let navStack = NavStackUtil.getNavStack();
    return navStack?.getParamByIndex(navStack.size() - 1) as T
  }

  /**
   * 页面跳转
   * @param info 跳转页面的信息
   * @param isSingle 是否为单例跳转，默认 false
   * @param launchMode 跳转模式，默认为 standard,
   *  针对短时间内 pop 和 push 一起触发时，可以使用 newInstance 模式触发切换动画
   * */
  static jumpPush(info: NavPathInfo, isSingle = false, launchMode = LaunchMode.STANDARD) {
    let navStack = NavStackUtil.getNavStack();
    if (isSingle) {
      if (navStack?.moveToTop(info.name) !== -1) {
        return;
      }
    }
    navStack.pushPath(info, { launchMode: launchMode });
  }

  /**
   * 页面返回
   * @param info 指定返回的页面信息
   * @param result 返回页面携带的数据（返回上一个页面才生效）
   * */
  static backPop(info?: NavPathInfo, result?: Object) {
    let navStack = NavStackUtil.getNavStack();
    if (info) {
      navStack.popToName(info.name, result);
    } else {
      navStack.pop(result);
    }
  }

  static getNavStack() {
    return AppStorageV2.connect(NavPathStack, NAV_STACK, () => {
      return new NavPathStack()
    })!;
  }
}