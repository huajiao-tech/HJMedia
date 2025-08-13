export class ToastUtils {
  static context: UIContext

  static init(context: UIContext) {
    ToastUtils.context = context
  }

  static showToast(message: string) {
    ToastUtils.context.getPromptAction().showToast({
      message
    })
  }
}