import {
  HJPlayerSourceType, HJPlayerType, HJPlayerVideoCodecType, SetWindowState
} from 'hjplayer';
import { HJRteGraphConfig } from 'hjplayer/src/main/ets/native/HJRteGraphSetupInfo';
import {
  OpenAction,
  PlayerXComponentState,
  PlayerXComponentStore,
  WindowAction
} from '../store/PlayerXComponentStore';

class PlayerXComponentController extends XComponentController {
  private playerXComponentStore: PlayerXComponentStore

  constructor(playerXComponentStore: PlayerXComponentStore) {
    super();
    this.playerXComponentStore = playerXComponentStore
  }

  onSurfaceCreated(surfaceId: string): void {
    this.playerXComponentStore.dispatch(new WindowAction({
      classStyle:HJRteGraphConfig.HJNodeClass_TargetUI_0,
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      surfaceId: surfaceId,
      width: 1,
      height: 1,
      state: SetWindowState.TARGET_CREATE
    }))
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {
    this.playerXComponentStore.dispatch(new WindowAction({
      classStyle:HJRteGraphConfig.HJNodeClass_TargetUI_0,
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      surfaceId: surfaceId,
      width: rect.surfaceWidth,
      height: rect.surfaceHeight,
      state: SetWindowState.TARGET_CHANGE
    }))
  }

  onSurfaceDestroyed(surfaceId: string) {
    this.playerXComponentStore.dispatch(new WindowAction({
      classStyle:HJRteGraphConfig.HJNodeClass_TargetUI_0,
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      surfaceId: surfaceId,
      width: 0,
      height: 0,
      state: SetWindowState.TARGET_DESTROY
    }))
  }
}

@ComponentV2
export struct PlayerXComponent {
  @Param @Require url: string = ""
  @Param isHLDS: boolean = false
  @Param isVOD: boolean = false
  @Param localdir: string = "/data/storage/el2/base/haps/entry/files/hjmedias"
  @Param rid: string = ""
  @Param sourceType: HJPlayerSourceType = HJPlayerSourceType.HJPlayerSourceType_SERIES
  @Param playerType: HJPlayerType = HJPlayerType.HJPlayerType_LIVESTREAM
  playerXComponentStore: PlayerXComponentStore = new PlayerXComponentStore()
  state: PlayerXComponentState = this.playerXComponentStore.getState()
  playerXComponentController: PlayerXComponentController = new PlayerXComponentController(this.playerXComponentStore)

  aboutToAppear(): void {
    const hjurl = this.isHLDS ? "hjds:" + this.url : this.url;
    //fixme lfs
    let playerType = HJPlayerType.HJPlayerType_VOD;
    if (!this.isVOD && (hjurl.startsWith("http://") || hjurl.startsWith("https://"))) {
      playerType = HJPlayerType.HJPlayerType_LIVESTREAM;
    }

    this.playerXComponentStore.init({
      url: hjurl, localDir: this.localdir, rid: this.rid,
      fps: 30,
      repeats: 0,
      videoCodecType: HJPlayerVideoCodecType.HJPlayerVideoCodecType_OHCODEC,
      sourceType: this.sourceType,
      playerType: playerType,
      bSplitScreenMirror: false,
      disableMFlag: 0,
      m_graphConfig:'',
      m_enableSEIUUids: new Map<string, boolean>([
        // ["12345678123456781234567812345678", false],
        // ["12345678123456781234567812345679", false],
      ]),
    })
    this.playerXComponentStore.dispatch(new OpenAction())
  }

  aboutToDisappear(): void {
    this.playerXComponentStore.aboutToDisappear()
  }

  build() {
    XComponent({
      type: XComponentType.SURFACE,
      controller: this.playerXComponentController
    })
      .position({ x: this.state.x, y: this.state.y })
      .width(this.state.w)
      .height(this.state.h)
      .backgroundColor("#00ffffff")
  }
}
