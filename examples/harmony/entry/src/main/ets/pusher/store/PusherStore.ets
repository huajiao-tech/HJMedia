import { State, Store, Action } from '../../interface/Store';
import { MediaConfig } from '../model/PusherModel';
import { AppStorageV2 } from '@kit.ArkUI';
import { PusherConfigPreset, pusherPreset } from '../model/PusherPreviewModel';
import { NavStackUtil } from '../../util/NavStackUitl';
import { abilityAccessCtrl } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { ToastUtils } from '../../util/ToastUtils';

@ObservedV2
export class PusherState implements State {
  @Trace pusherConfigArray: MediaConfig[] = [
    new MediaConfig("string", 'URL 推流地址', 'rtmp://live-push-1.huajiao.com/main/lfs?auth_key=1762482963-0-0-350b9e1e3cde5ec152a628888169a6f6'),
    new MediaConfig("boolean", '是否横屏', false),
    new MediaConfig("boolean", '是否保持屏幕常亮', false),
    new MediaConfig("boolean", '是否双屏预览', false),
    new MediaConfig("slider", 'ROI', -3, -51, 51),
  ]
  config: PusherConfigPreset = AppStorageV2.connect(PusherConfigPreset, () => pusherPreset)!
}

export class PreviewAction implements Action<PusherState> {
}

export class PusherStore implements Store<PusherState> {
  state = new PusherState()
  getState = () => this.state

  dispatch(action: Action<PusherState>): void {
    if (action instanceof PreviewAction) {
      this.state.config.url = this.state.pusherConfigArray[0].value as string
      this.state.config.isLandScape = this.state.pusherConfigArray[1].value as boolean
      this.state.config.isScreenOn = this.state.pusherConfigArray[2].value as boolean

      this.state.config.roiQuantOffset = this.state.pusherConfigArray[4].value as number
      console.log("roiQuantOffset: ", this.state.pusherConfigArray[4].value);

      if (this.state.config.isLandScape) {
        this.state.config.videoConfig.width = 1280
        this.state.config.videoConfig.height = 720
      } else {
        this.state.config.videoConfig.width = 720
        this.state.config.videoConfig.height = 1280
      }
      try {
        let atManager = abilityAccessCtrl.createAtManager();
        atManager.requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA', 'ohos.permission.MICROPHONE'])
          .then((data) => {
            if (data.authResults) {
              if (data.authResults.every(item => item == 0)) {
                NavStackUtil.jumpPush({ name: "PusherPreviewPage", param: this.state.pusherConfigArray[3].value as boolean })
              } else {
                ToastUtils.showToast("请进入设置中允许相机和麦克风权限后重新进入!")
              }
            }
            // console.log('Entry', 'requestPre() data: ' + JSON.stringify(data));
          }).catch((err: BusinessError) => {
          // Logger.info('Entry', 'requestPre() data: ' + JSON.stringify(err));
        })
      } catch (err) {
        // Logger.error('Entry', 'requestPre() data: ' + JSON.stringify(err));
      }
    }
  }
}