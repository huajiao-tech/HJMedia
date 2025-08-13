import { HJPusher } from 'hjpusher';
import { SetWindowState } from 'hjpusher';
import { PusherXComponentState, PusherXComponentStore } from '../store/PusherXComponentStore';

class PusherXComponentController extends XComponentController {
  private hjPusher: HJPusher

  constructor(hjPusher: HJPusher) {
    super();
    this.hjPusher = hjPusher;
  }

  onSurfaceCreated(surfaceId: string): void {
    this.hjPusher.setWindow({
      surfaceId: surfaceId,
      width: 1,
      height: 1,
      state: SetWindowState.TARGET_CREATE
    })
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {
    this.hjPusher.setWindow({
      surfaceId: surfaceId,
      width: rect.surfaceWidth,
      height: rect.surfaceHeight,
      state: SetWindowState.TARGET_CHANGE
    })
  }

  async onSurfaceDestroyed(surfaceId: string) {

  }
}

@ComponentV2
export struct PusherXComponent{
  @Param @Require hjPusher: HJPusher = new HJPusher();
  pusherXComponentStore: PusherXComponentStore = new PusherXComponentStore()
  state: PusherXComponentState = this.pusherXComponentStore.getState()
  pusherXComponentController: PusherXComponentController = new PusherXComponentController(this.hjPusher);

  build() {
    XComponent({
      id: 'PusherXComponent',
      type: XComponentType.SURFACE,
      controller: this.pusherXComponentController
    })
      .position({ x: this.state.x, y: this.state.y })
      .width(this.state.w)
      .height(this.state.h)
  }
}
