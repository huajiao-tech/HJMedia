import { CameraDataModel } from '../model/CameraDateModel';
import { camera } from '@kit.CameraKit';
import { BusinessError } from '@kit.BasicServicesKit';
import Logger from '../common/utils/Logger';
import { profileCameraCheck } from '../common/utils/CameraCheck';
import { setColorSpaceBeforeCommitConfig, setVideoStabilizationMode } from '../common/utils/cameraProfile';
import { display } from '@kit.ArkUI';
import { JSON } from '@kit.ArkTS';

const TAG: string = "CameraService"

class CameraService {
  videoSession?: camera.VideoSession;
  private cameraDataModel: CameraDataModel = new CameraDataModel()
  private cameraManager?: camera.CameraManager
  private cameraDevices: camera.CameraDevice[] = []
  private cameraInput?: camera.CameraInput;
  private previewOutput?: camera.PreviewOutput;
  private surfaceId?: string;
  private videoSessionState: "start" | "stop" = "stop"
  private startNum: number = 0
  private promise: Promise<void> | null = null

  private beginConfig() {
    try {
      this.videoSession?.beginConfig();
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `Failed to beginConfig. error: ${JSON.stringify(err)}`);
    }
  }

  private async createOutput(cameraDevice: camera.CameraDevice) {
    const previewProfile = profileCameraCheck(this.cameraManager!, this.cameraDataModel, cameraDevice)
    try {
      this.previewOutput = this.cameraManager?.createPreviewOutput(previewProfile, this.surfaceId);
    } catch (error) {
      let err = error as BusinessError;
      console.error(TAG, "Failed to create the PreviewOutput instance. error code: " + err.code);
    }
    if (this.previewOutput === undefined) {
      Logger.error(TAG, 'XComponentPreviewOutput is undefined');
      return;
    }
  }

  private addInput() {
    try {
      this.videoSession?.addInput(this.cameraInput);
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `Failed to add cameraInput. error: ${JSON.stringify(err)}`);
    }
  }

  private addOutput() {
    try {
      this.videoSession?.canAddOutput(this.previewOutput);
    } catch (error) {
      let err = error as BusinessError;
      console.error(TAG, `Failed to add previewOutput. error: ${err}`);
    }
    try {
      this.videoSession?.addOutput(this.previewOutput);
    } catch (error) {
      let err = error as BusinessError;
      console.error(TAG, `Failed to add previewOutput. error: ${err}`);
    }
  }

  private async startSession() {
    try {
      await this.videoSession?.commitConfig();
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `videoSession commitConfig error: ${JSON.stringify(err)}`);
    }

    if (this.previewOutput)
    {
      let initDisplayRotation = display.getDefaultDisplaySync().rotation;
      let initPreviewRotation : camera.ImageRotation = this.previewOutput.getPreviewRotation(initDisplayRotation * camera.ImageRotation.ROTATION_90);
      this.previewOutput.setPreviewRotation(initPreviewRotation, false);

      console.log("cameratest displayRotation: " + initDisplayRotation);
      console.log("cameratest preview: " + initPreviewRotation);
    }
    if (this.videoSession) {
      if (setVideoStabilizationMode(this.videoSession)) {
        setColorSpaceBeforeCommitConfig(this.videoSession, this.cameraDataModel.isHDRVivid);
      }
      // this.videoSession.on('controlCenterEffectStatusChange',
      //   (err: BusinessError, status: camera.ControlCenterStatusInfo) => {
      //     console.log(TAG, JSON.stringify(err), JSON.stringify(status))
      //   })
    }

    await this.startVideoSession()
  }

  private async startVideoSession() {
    if (this.videoSessionState == "start") {
      return
    }
    try {
      this.videoSessionState = "start"
      await this.videoSession?.start();
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `videoSession start error: ${JSON.stringify(err)}`);
    }
  }

  private async stopVideoSession() {
    if (this.videoSessionState == "stop") {
      return
    }
    try {
      this.videoSessionState = "stop"
      await this.videoSession?.stop();
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `Failed to stop. error: ${JSON.stringify(err)}`);
    }
  }

  private isTorchSupported(): boolean {
    let torchSupport: boolean = false;
    try {
      torchSupport = this.cameraManager?.isTorchSupported() ?? false;
    } catch (error) {
      let err = error as BusinessError;
      console.error('Failed to torch. errorCode = ' + err.code);
    }
    console.info('Returned with the torch support status:' + torchSupport);
    return torchSupport;
  }

  private isTorchModeSupported(torchMode: camera.TorchMode): boolean {
    let isTorchModeSupport: boolean = false;
    try {
      isTorchModeSupport = this.cameraManager?.isTorchModeSupported(torchMode) ?? false;
    } catch (error) {
      let err = error as BusinessError;
      console.error('Failed to set the torch mode. errorCode = ' + err.code);
    }
    return isTorchModeSupport;
  }

  private onTorchStatusChange(): void {
    this.cameraManager?.on('torchStatusChange', (err: BusinessError, torchStatusInfo: camera.TorchStatusInfo) => {
      if (err !== undefined && err.code !== 0) {
        console.error(`Callback Error, errorCode: ${err.code}`);
        return;
      }
      console.info(`onTorchStatusChange, isTorchAvailable: ${torchStatusInfo.isTorchAvailable}, isTorchActive: ${torchStatusInfo.isTorchActive}, level: ${torchStatusInfo.torchLevel}`);
    });
  }

  private setTorchModeSupported(torchMode: camera.TorchMode): void {
    this.cameraManager?.setTorchMode(torchMode);
    let isTorchMode = this.cameraManager?.getTorchMode();
    console.info(`Returned with the torch mode supportd mode: ${isTorchMode}`);
  }

  public openTorch(): boolean {
    if (!this.isTorchSupported()) {
      Logger.error(TAG, 'torch is not supported');
      return false;
    }
    if (!this.isTorchModeSupported(camera.TorchMode.ON)) {
      Logger.error(TAG, 'torch mode is not supported');
      return false;
    }
    this.onTorchStatusChange()
    this.setTorchModeSupported(camera.TorchMode.ON)
    return true
  }

  private hasFlash(): boolean {
    let status: boolean = false;
    try {
      status = this.videoSession?.hasFlash() ?? false;
    } catch (error) {
      let err = error as BusinessError;
      console.error(`The hasFlash call failed. error code: ${err.code}`);
    }
    return status;
  }

  private isFlashModeSupported(mode: camera.FlashMode): boolean {
    let status: boolean = false;
    try {
      status = this.videoSession?.isFlashModeSupported(mode) ?? false;
    } catch (error) {
      let err = error as BusinessError;
      console.error(`The isFlashModeSupported call failed. error code: ${err.code}`);
    }
    return status;
  }

  public openFlash(): boolean {
    if (!this.hasFlash()) {
      Logger.error(TAG, 'flash is not supported');
      return false;
    }
    if (!this.isFlashModeSupported(camera.FlashMode.FLASH_MODE_ALWAYS_OPEN)) {
      Logger.error(TAG, 'flash mode is not supported');
      return false;
    }
    this.videoSession?.setFlashMode(camera.FlashMode.FLASH_MODE_ALWAYS_OPEN)
    return true
  }

  public closeFlash(): boolean {
    if (!this.hasFlash()) {
      Logger.error(TAG, 'flash is not supported');
      return false;
    }
    if (!this.isFlashModeSupported(camera.FlashMode.FLASH_MODE_CLOSE)) {
      Logger.error(TAG, 'flash mode is not supported');
      return false;
    }
    this.videoSession?.setFlashMode(camera.FlashMode.FLASH_MODE_CLOSE)
    return true
  }

  public closeTorch(): boolean {
    if (!this.isTorchSupported()) {
      Logger.error(TAG, 'torch is not supported');
      return false;
    }
    if (!this.isTorchModeSupported(camera.TorchMode.OFF)) {
      Logger.error(TAG, 'torch mode is not supported');
      return false;
    }
    this.onTorchStatusChange()
    this.setTorchModeSupported(camera.TorchMode.OFF)
    return true
  }

  /**
   * 初始化相机
   * @param context
   * @param cameraDataModel
   */
  public initCamera(context: Context, cameraDataModel?: CameraDataModel) {
    if (cameraDataModel) {
      this.cameraDataModel = cameraDataModel
    }
    try {
      this.cameraManager = camera.getCameraManager(context);
    } catch (error) {
      Logger.error(TAG, `getCameraManager error, errCode: ${error.code}`);
      return;
    }
    this.cameraDevices = this.getCameraDevices()
    this.cameraManager.on('cameraStatus', (err: BusinessError, cameraStatusInfo: camera.CameraStatusInfo) => {
      if (err !== undefined && err.code !== 0) {
        console.error(TAG, `Callback Error, errorCode: ${err.code}`);
        return;
      }
      // 如果当通过USB连接相机设备时，回调函数会返回新的相机出现状态。
      if (cameraStatusInfo.status == camera.CameraStatus.CAMERA_STATUS_APPEAR) {
        console.info(TAG, `New Camera device appear.`);
      }
      // 如果当断开相机设备USB连接时，回调函数会返回相机被移除状态。
      if (cameraStatusInfo.status == camera.CameraStatus.CAMERA_STATUS_DISAPPEAR) {
        console.info(TAG, `Camera device has been removed.`);
      }
      // 相机被关闭时，回调函数会返回相机可用状态。
      if (cameraStatusInfo.status == camera.CameraStatus.CAMERA_STATUS_AVAILABLE) {
        console.info(TAG, `camera: ${cameraStatusInfo.camera.cameraId} status: ${cameraStatusInfo.status} Current Camera is available.`);
      }
      // 相机被打开/占用时，回调函数会返回相机不可用状态。
      if (cameraStatusInfo.status == camera.CameraStatus.CAMERA_STATUS_UNAVAILABLE) {
        console.info(TAG, `camera: ${cameraStatusInfo.camera.cameraId} status: ${cameraStatusInfo.status} Current Camera has been occupied.`);
      }
      // console.info(TAG, `camera: ${cameraStatusInfo.camera.cameraId}`);
      // console.info(TAG, `status: ${cameraStatusInfo.status}`);
    });
  }

  private getCameraDevices(): Array<camera.CameraDevice> {
  let cameraArray: Array<camera.CameraDevice> = this.cameraManager?.getSupportedCameras() ?? [];
  if (cameraArray != undefined && cameraArray.length > 0) {
    for (let index = 0; index < cameraArray.length; index++) {
      console.info(TAG, 'cameraInfo: ' + JSON.stringify(cameraArray[index]));
    }
    return cameraArray;
  } else {
    console.error(TAG, "cameraManager.getSupportedCameras error");
    return [];
  }
}

  public bindSurfaceId(surfaceId: string) {
    this.surfaceId = surfaceId
  }

  private async doReleaseCamera(): Promise<void> {
    await this.stopVideoSession()
    try {
      await this.cameraInput?.close()
      await this.previewOutput?.release()
      await this.videoSession?.release()
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `releaseCamera start error: ${JSON.stringify(err)}`);
    }
  }

  /**
   * 释放相机关闭输入输出
   * @returns
   */
  public async releaseCamera() {
    if (this.startNum == 0) {
      this.startNum++
    } else {
      this.startNum = 0
      this.promise = this.doReleaseCamera()
    }
  }

  private async createInput(cameraDevice: camera.CameraDevice) {
    try {
      this.cameraInput = this.cameraManager?.createCameraInput(cameraDevice);
    } catch (error) {
      let err = error as BusinessError;
      console.error('Failed to createCameraInput errorCode = ' + err.code);
    }
    // 监听cameraInput错误信息。
    this.cameraInput?.on('error', cameraDevice, (error: BusinessError) => {
      console.error(`Camera input error code: ${error.code}`);
    });
    try {
      await this.cameraInput?.open();
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `Failed to open cameraInput. error: ${JSON.stringify(err)}`);
    }
  }

  /**
   * 开启相机并预览
   * @param position
   * @returns
   */
  public async startPreview(position: number): Promise<void> {
    if (this.promise) {
      await this.promise
    }
    this.startNum++
    if (this.startNum > 1) {
      this.startNum = 0
      await this.doReleaseCamera()
    }
    await this.createOutput(this.cameraDevices[position])
    this.createInput(this.cameraDevices[position])

    try {
      this.videoSession = this.cameraManager?.createSession(camera.SceneMode.NORMAL_VIDEO) as camera.VideoSession;
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `Failed to create the session instance. error: ${JSON.stringify(err)}`);
    }
    if (this.videoSession === undefined) {
      Logger.error(TAG, 'videoSession is undefined');
      return;
    }
    this.beginConfig()
    this.addInput()
    this.addOutput()
    this.startSession()
  }

  public async stopPreview() {
    await this.stopVideoSession()
  }

  /**
   * 切换相机
   * @param position 相机位置
   */
  public async switchCamera(position: number) {
    await this.stopVideoSession()
    this.beginConfig()
    try {
      this.videoSession?.removeOutput(this.previewOutput)
      await this.previewOutput?.release()
    } catch (error) {
      const err = error as BusinessError;
      Logger.error(TAG, `Failed to removeOutput. error: ${JSON.stringify(err)}`);
    }
    try {
      this.videoSession?.removeInput(this.cameraInput)
    } catch (error) {
      const err = error as BusinessError;
      Logger.error(TAG, `Failed to removeInput. error: ${JSON.stringify(err)}`);
    }
    try {
      await this.cameraInput?.close()
    } catch (error) {
      let err = error as BusinessError;
      Logger.error(TAG, `Failed to close. error: ${JSON.stringify(err)}`);
    }
    this.createInput(this.cameraDevices[position])
    await this.createOutput(this.cameraDevices[position])
    this.addInput()
    this.addOutput()
    this.startSession()
  }

  // public openControlCenter() {
  //   let effectTypes: Array<camera.ControlCenterEffectType> = []
  //   effectTypes = this.videoSession?.getSupportedEffectTypes() ?? []
  //   let isSupported: boolean = this.videoSession?.isControlCenterSupported() ?? false
  //   if (isSupported) {
  //     this.videoSession?.enableControlCenter(true);
  //   }
  // }
}

export const cameraService = new CameraService();