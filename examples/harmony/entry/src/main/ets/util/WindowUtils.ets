import { window } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';

export class WindowUtils {
  static window?: window.Window

  static init(windowClass:  window.Window) {
    WindowUtils.window = windowClass
  }

  static setOrientation(orientation: window.Orientation) {
    try {
      WindowUtils.window?.setPreferredOrientation(orientation, (err: BusinessError) => {
        const errCode: number = err.code;
        if (errCode) {
          console.error(`Failed to set window orientation. Cause code: ${err.code}, message: ${err.message}`);
          return;
        }
        console.info('Succeeded in setting window orientation.');
      });
    } catch (exception) {
      console.error(`Failed to set window orientation. Cause code: ${exception.code}, message: ${exception.message}`);
    }
  }

  static setWindowLayoutFullScreen(isLayoutFullScreen: boolean) {
    WindowUtils.window?.setWindowLayoutFullScreen(isLayoutFullScreen)
  }

  static async setWindowKeepScreenOn(isKeepScreenOn: boolean) {
    try {
      WindowUtils.window?.setWindowKeepScreenOn(isKeepScreenOn, (err: BusinessError) => {
        const errCode: number = err.code;
        if (errCode) {
          console.error('Failed to set the screen to be always on. Cause: ' + JSON.stringify(err));
          return;
        }
        console.info('Succeeded in setting the screen to be always on.');
      });
    } catch (exception) {
      console.error('Failed to set the screen to be always on. Cause: ' + JSON.stringify(exception));
    }
  }
}