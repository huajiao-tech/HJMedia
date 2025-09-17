import { State, Store, Action } from '../../interface/Store';
import { MediaConfig } from '../../pusher/model/PusherModel';
import { NavStackUtil } from '../../util/NavStackUitl';
import { WindowUtils } from '../../util/WindowUtils';

@ObservedV2
export class PlayerSettingState implements State {
  @Trace playerConfigArray: MediaConfig[] = [
    new MediaConfig("string", 'URL 播放地址', 'http'),
    new MediaConfig("boolean", '是否横屏', false),
    // new MediaConfig("boolean", '是否保持屏幕常亮', false),
  ]
}

export class PlayerSettingStore implements Store<PlayerSettingState> {
  state = new PlayerSettingState()
  getState = () => this.state

  onPageShow(): void {
    WindowUtils.setWindowLayoutFullScreen(true)
  }

  onPageHide(): void {
    WindowUtils.setWindowLayoutFullScreen(false)
  }

  dispatch(action: Action<PlayerSettingState>): void {
    if (action instanceof PlayAction) {
      NavStackUtil.jumpPush({ name: "LivePlayerPage", param: this.state.playerConfigArray[1].value as boolean })
    }
  }
}

export class PlayAction implements Action<PlayerSettingState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}