import { HJPusher } from 'hjpusher';
import { SetWindowState } from 'hjpusher';
import { HJRteGraphConfig } from 'hjpusher/src/main/ets/native/HJRteGraphSetupInfo';
import { cameraService } from '../../camera/service/CameraService';
import { PusherXComponentState, PusherXComponentStore } from '../store/PusherXComponentStore';
import { window } from '@kit.ArkUI';

class PusherXComponentController extends XComponentController {
  private hjPusher: HJPusher

  constructor(hjPusher: HJPusher) {
    super();
    this.hjPusher = hjPusher;
  }

  onSurfaceCreated(surfaceId: string): void {
    this.hjPusher.setWindow({
      classStyle:HJRteGraphConfig.HJNodeClass_TargetUI_0, //HJNodeClass_TargetUI_1
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,   //HJNodeClass_TargetUI_1
      surfaceId: surfaceId,
      width: 1,
      height: 1,
      state: SetWindowState.TARGET_CREATE
    })
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {
    this.hjPusher.setWindow({
      classStyle:HJRteGraphConfig.HJNodeClass_TargetUI_0, //HJNodeClass_TargetUI_1
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,   //HJNodeClass_TargetUI_1
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
  context: Context | undefined = this.getUIContext().getHostContext()
  windowClass: window.Window | undefined = undefined

  async aboutToAppear() {
    try {
      const windowClass = await window.getLastWindow(this.context as Context)
      this.windowClass = windowClass
      windowClass.on('windowSizeChange', this.onWindowSizeChange)
    } catch (error) {
      console.error(`Failed to getLastWindow. error: ${JSON.stringify(error)}`)
    }
  }

  aboutToDisappear() {
    try {
      this.windowClass?.off('windowSizeChange', this.onWindowSizeChange)
    } catch (error) {
      console.error(`Failed to getLastWindow. error: ${JSON.stringify(error)}`)
    }
  }

  private onWindowSizeChange = (size: window.Size) => {
    this.updateCameraPreviewRotation()
  }

  private updateCameraPreviewRotation() {
    const initPreviewRotation = cameraService.updatePreviewRotation();
    if (initPreviewRotation !== undefined) {
      this.hjPusher.setPreviewRotation(initPreviewRotation)
    }
  }

  build() {
    XComponent({
      type: XComponentType.SURFACE,
      controller: this.pusherXComponentController
    })
      .position({ x: this.state.x, y: this.state.y })
      .width(this.state.w)
      .height(this.state.h)
  }
}
