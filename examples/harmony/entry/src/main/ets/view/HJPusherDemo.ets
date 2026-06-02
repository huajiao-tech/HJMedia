import { abilityAccessCtrl } from '@kit.AbilityKit';
import { AppStorageV2, window } from '@kit.ArkUI';
import common from '@ohos.app.ability.common';
import {
  AudioConfig,
  AudioCodecType,
  HJPusher,
  HJRteGraphConfig,
  LogMode,
  PreviewInfo,
  PusherConfig,
  RecorderInfo,
  SetWindowState,
  VideoConfig,
  VideoCodecType
} from 'hjpusher';
import { HJRteGraphConfigConstructor, HJRteGraphConstructorType } from 'hjpusher/src/main/ets/native/HJRteGraphConfigConstructor';
import { HJRteAdjacentNodeInfo, HJRteJsonLinkInfo } from 'hjpusher/src/main/ets/native/HJRteGraphSetupInfo';
import { MediaStateInfo } from 'hjpusher/src/main/ets/native/HJCommTypes';
import { HJFaceDetectMgr, HJFaceDetectMgrRet, HJNativeSourceData } from 'hjpusher/src/main/ets/HJFaceDetectMgr';
import { cameraService } from '../camera/service/CameraService';
import { CommonConstant as Common } from '../common/CommonConstant';
import { PusherConfigPreset, pusherPreset } from '../pusher/model/PusherPreviewModel';
import { WindowUtils } from '../util/WindowUtils';
import { hjCommonDemoOption, hjPusherDemoOption, HJCommonDemoOption, HJPusherDemoOption } from './HJDemoOption';

class DemoVideoSize {
  width: number = 0;
  height: number = 0;

  constructor(width: number, height: number) {
    this.width = width;
    this.height = height;
  }
}

class DemoPreviewSize {
  realWidth: number = 0;
  realHeight: number = 0;

  constructor(realWidth: number, realHeight: number) {
    this.realWidth = realWidth;
    this.realHeight = realHeight;
  }
}

interface DemoSelectOption {
  value: string;
}

enum DemoActionType {
  Preview = 'preview',
  Encoding = 'encoding',
  Record = 'record',
  Camera = 'camera',
  FaceDetect = 'faceDetect',
  Faceu = 'faceu',
  GiftPng = 'giftPng',
  FaceProtected = 'faceProtected'
}

enum DemoRandomTestType {
  Off = 'off',
  Preview = 'preview',
  Encoding = 'encoding',
  Faceu = 'faceu',
  FaceDetect = 'faceDetect',
  Window = 'window',
  Mode = 'mode',
  All = 'all'
}

class HJPusherDemoXComponentController extends XComponentController {
  private readonly hjPusher: HJPusher;
  private currentSurfaceId: string = '';
  private currentWidth: number = 1;
  private currentHeight: number = 1;
  private currentState: SetWindowState = SetWindowState.TARGET_CREATE;
  private isPreviewReady: boolean = false;
  private lastSkipReason: string = '';

  constructor(hjPusher: HJPusher) {
    super();
    this.hjPusher = hjPusher;
  }

  private priLogWindowOp(stage: string, state: SetWindowState, surfaceId: string, width: number, height: number, result: number): void {
    console.info(`[HJPusherDemo] ${stage} setWindow state=${state} surfaceId=${surfaceId} width=${width} height=${height} result=${result}`);
  }

  private priLogWindowSkip(stage: string, reason: string): void {
    if (this.lastSkipReason === `${stage}:${reason}`) {
      return;
    }
    this.lastSkipReason = `${stage}:${reason}`;
    console.info(`[HJPusherDemo] ${stage} skip setWindow reason=${reason} previewReady=${this.isPreviewReady} surfaceId=${this.currentSurfaceId} width=${this.currentWidth} height=${this.currentHeight} state=${this.currentState}`);
  }

  onSurfaceCreated(surfaceId: string): void {
    this.currentSurfaceId = surfaceId;
    this.currentWidth = 1;
    this.currentHeight = 1;
    this.currentState = SetWindowState.TARGET_CREATE;
    this.lastSkipReason = '';
    console.info(`[HJPusherDemo] onSurfaceCreated surfaceId=${surfaceId}`);
    this.priApplySavedSurfaceIfReady();
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {
    this.currentSurfaceId = surfaceId;
    this.currentWidth = rect.surfaceWidth;
    this.currentHeight = rect.surfaceHeight;
    this.currentState = SetWindowState.TARGET_CHANGE;
    this.lastSkipReason = '';
    console.info(`[HJPusherDemo] onSurfaceChanged surfaceId=${surfaceId} width=${rect.surfaceWidth} height=${rect.surfaceHeight}`);
    this.priApplySavedSurfaceIfReady();
  }

  onSurfaceDestroyed(surfaceId: string): void {
    if (this.isPreviewReady) {
      const result = this.hjPusher.setWindow({
        classStyle: HJRteGraphConfig.HJNodeClass_TargetUI_0,
        insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
        surfaceId: surfaceId,
        width: 0,
        height: 0,
        state: SetWindowState.TARGET_DESTROY
      });
      this.priLogWindowOp('onSurfaceDestroyed', SetWindowState.TARGET_DESTROY, surfaceId, 0, 0, result);
    }
    console.info(`[HJPusherDemo] onSurfaceDestroyed surfaceId=${surfaceId}`);
    this.currentSurfaceId = '';
    this.currentWidth = 1;
    this.currentHeight = 1;
    this.currentState = SetWindowState.TARGET_DESTROY;
    this.isPreviewReady = false;
    this.lastSkipReason = '';
  }

  hasSurface(): boolean {
    return this.currentSurfaceId.length > 0;
  }

  notifyPreviewOpened(): void {
    this.isPreviewReady = true;
    this.lastSkipReason = '';
    console.info(`[HJPusherDemo] notifyPreviewOpened surfaceReady=${this.currentSurfaceId.length > 0} cachedState=${this.currentState} width=${this.currentWidth} height=${this.currentHeight}`);
    this.priApplySavedSurfaceOnPreviewOpen();
  }

  notifyPreviewClosed(): void {
    if (this.currentSurfaceId) {
      const result = this.hjPusher.setWindow({
        classStyle: HJRteGraphConfig.HJNodeClass_TargetUI_0,
        insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
        surfaceId: this.currentSurfaceId,
        width: 0,
        height: 0,
        state: SetWindowState.TARGET_DESTROY
      });
      this.priLogWindowOp('notifyPreviewClosed', SetWindowState.TARGET_DESTROY, this.currentSurfaceId, 0, 0, result);
    } else {
      this.priLogWindowSkip('notifyPreviewClosed', 'surface_missing');
    }
    this.isPreviewReady = false;
    this.lastSkipReason = '';
    console.info('[HJPusherDemo] notifyPreviewClosed');
  }

  private priApplySavedSurfaceIfReady(): void {
    if (!this.isPreviewReady) {
      this.priLogWindowSkip('applySavedSurfaceIfReady', 'preview_not_ready');
      return;
    }
    if (!this.currentSurfaceId) {
      this.priLogWindowSkip('applySavedSurfaceIfReady', 'surface_missing');
      return;
    }
    this.lastSkipReason = '';
    const result = this.hjPusher.setWindow({
      classStyle: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      surfaceId: this.currentSurfaceId,
      width: this.currentWidth,
      height: this.currentHeight,
      state: this.currentState
    });
    this.priLogWindowOp('applySavedSurfaceIfReady', this.currentState, this.currentSurfaceId, this.currentWidth, this.currentHeight, result);
  }

  private priApplySavedSurfaceOnPreviewOpen(): void {
    if (!this.isPreviewReady) {
      this.priLogWindowSkip('applySavedSurfaceOnPreviewOpen', 'preview_not_ready');
      return;
    }
    if (!this.currentSurfaceId) {
      this.priLogWindowSkip('applySavedSurfaceOnPreviewOpen', 'surface_missing');
      return;
    }
    this.lastSkipReason = '';
    const createResult = this.hjPusher.setWindow({
      classStyle: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      surfaceId: this.currentSurfaceId,
      width: this.currentWidth,
      height: this.currentHeight,
      state: SetWindowState.TARGET_CREATE
    });
    this.priLogWindowOp('applySavedSurfaceOnPreviewOpen', SetWindowState.TARGET_CREATE, this.currentSurfaceId, this.currentWidth, this.currentHeight, createResult);
    if (this.currentState === SetWindowState.TARGET_CHANGE) {
      this.priApplySavedSurfaceIfReady();
    }
  }
}

@Component
export struct HJPusherDemo {
  @State isPreviewOpen: boolean = false;
  @State isRenderSurfaceMounted: boolean = true;
  @State renderSurfaceVersion: number = 0;
  @State xComponentWidth: Resource = $r('app.float.xcomponent_width');
  @State xComponentHeight: Resource = $r('app.float.xcomponent_height');
  @State isPreviewFitMode: boolean = false;
  @State isEncodingOpen: boolean = false;
  @State isLandscape: boolean = false;
  @State isFrontCamera: boolean = true;
  @State showSettingsDialog: boolean = false;
  @State showFaceuDialog: boolean = false;
  @State showPngGiftDialog: boolean = false;
  @State pendingEncodingStart: boolean = false;
  @State currentStatus: string = '预览未开启';
  @State urlDraft: string = '';
  @State faceuUrlDraft: string = '';
  @State pngGiftUrlDraft: string = '';
  @State roiEnabledDraft: boolean = false;
  @State roiOffsetDraft: number = -3;
  @State videoCodecDraft: number = VideoCodecType.HJVCodecH265;
  @State hasCameraPermission: boolean = false;
  @State hasMicrophonePermission: boolean = false;
  @State isFaceDetectOpen: boolean = false;
  @State isFaceProtectedOpen: boolean = false;
  @State isFaceuOpen: boolean = false;
  @State isRecorderOpen: boolean = false;
  @State randomTestSelectedIndex: number = 0;
  @State isRandomTestRunning: boolean = false;

  private readonly hjPusher: HJPusher = new HJPusher();
  private readonly controller: HJPusherDemoXComponentController = new HJPusherDemoXComponentController(this.hjPusher);
  private readonly context: common.BaseContext = getContext(this) as common.BaseContext;
  private readonly config: PusherConfigPreset = AppStorageV2.connect(PusherConfigPreset, () => pusherPreset)!;
  private readonly commonDemoOption: HJCommonDemoOption = hjCommonDemoOption;
  private readonly pusherDemoOption: HJPusherDemoOption = hjPusherDemoOption;
  private readonly faceDetectMgr: HJFaceDetectMgr = new HJFaceDetectMgr();
  private readonly randomTestOptions: DemoSelectOption[] = [
    { value: '关闭随机' },
    { value: '随机预览' },
    { value: '随机推流' },
    { value: '随机萌颜' },
    { value: '随机检测' },
    { value: '随机窗口' },
    { value: '随机模式' },
    { value: '随机全部' }
  ];
  private readonly randomTestTypes: DemoRandomTestType[] = [
    DemoRandomTestType.Off,
    DemoRandomTestType.Preview,
    DemoRandomTestType.Encoding,
    DemoRandomTestType.Faceu,
    DemoRandomTestType.FaceDetect,
    DemoRandomTestType.Window,
    DemoRandomTestType.Mode,
    DemoRandomTestType.All
  ];
  private windowClass?: window.Window;
  private renderSurfaceRebuildTimer: number = -1;
  private randomTestTimer: number = -1;
  private randomTestActionInFlight: boolean = false;
  private windowSizeSwitchIndex: number = 0;
  private faceDetectRegistered: boolean = false;
  private faceDetectRet: number = HJFaceDetectMgrRet.ok;
  private readonly onWindowSizeChange = (_size: window.Size) => {
    this.priSyncPreviewRotation();
  };

  aboutToAppear(): void {
    console.info('[HJPusherDemo] aboutToAppear');
    this.isLandscape = false;
    this.config.videoConfig.videoIsROIEnc = false;
    WindowUtils.setOrientation(window.Orientation.PORTRAIT);
    this.priInitPusherContext();
    this.hjPusher.createPusher();
    void this.priAttachWindowListener();
  }

  aboutToDisappear(): void {
    console.info('[HJPusherDemo] aboutToDisappear');
    this.windowClass?.off('windowSizeChange', this.onWindowSizeChange);
    WindowUtils.setOrientation(window.Orientation.UNSPECIFIED);
    void this.priCleanup();
  }

  private priInitPusherContext(): void {
    const logLevel: number = 2;
    const logMode: number = LogMode.CONSOLE | LogMode.FILE;
    const logPath: string = '/data/storage/el2/base/haps/entry/files/HJPusherDemo';
    const logSize: number = 1024 * 1024 * 5;
    const logCount: number = 5;
    HJPusher.contextInit(true, logPath, logLevel, logMode, logSize, logCount);
  }

  private async priEnsurePermissions(): Promise<boolean> {
    try {
      const atManager = abilityAccessCtrl.createAtManager();
      const data = await atManager.requestPermissionsFromUser(getContext(), [
        'ohos.permission.CAMERA',
        'ohos.permission.MICROPHONE'
      ]);
      const authResults: number[] = data.authResults ?? [];
      if (authResults.length >= 2) {
        this.hasCameraPermission = authResults[0] === 0;
        this.hasMicrophonePermission = authResults[1] === 0;
      } else {
        const granted = authResults.length > 0 && authResults.every((item: number) => item === 0);
        this.hasCameraPermission = granted;
        this.hasMicrophonePermission = granted;
      }
      console.info(`[HJPusherDemo] ensurePermissions authResults=${JSON.stringify(authResults)} camera=${this.hasCameraPermission} mic=${this.hasMicrophonePermission}`);
      return this.hasCameraPermission && this.hasMicrophonePermission;
    } catch (err) {
      console.error(`[HJPusherDemo] requestPermissions failed: ${JSON.stringify(err)}`);
      this.hasCameraPermission = false;
      this.hasMicrophonePermission = false;
      return false;
    }
  }

  private priHasRequiredPermissions(): boolean {
    return this.hasCameraPermission && this.hasMicrophonePermission;
  }

  private async priEnsurePermissionsOrToast(): Promise<boolean> {
    const granted = await this.priEnsurePermissions();
    if (!granted) {
      this.priUpdateStatus('缺少相机或麦克风权限');
      this.priShowToast('请先允许相机和麦克风权限');
      return false;
    }
    return true;
  }

  private async priAttachWindowListener(): Promise<void> {
    try {
      this.windowClass = await window.getLastWindow(this.context);
      const rect = this.windowClass.getWindowProperties().windowRect;
      console.info(`[HJPusherDemo] attachWindowListener width=${rect.width} height=${rect.height} isLandscape=${this.isLandscape}`);
      this.windowClass.on('windowSizeChange', this.onWindowSizeChange);
    } catch (error) {
      console.error(`[HJPusherDemo] attachWindowListener failed: ${JSON.stringify(error)}`);
    }
  }

  private priBuildPreviewInfo(): PreviewInfo {
    const previewInfo = new PreviewInfo();
    const previewSize = this.priGetPreviewSourceSize();
    previewInfo.realWidth = previewSize.realWidth;
    previewInfo.realHeight = previewSize.realHeight;
    previewInfo.previewFps = this.config.videoConfig.frameRate;
    previewInfo.m_graphConfig = HJRteGraphConfigConstructor.constructGraph({
      type: HJRteGraphConstructorType.HJRteGraphConstructorType_PlaceHolder,
      isUI0Clip: !this.isPreviewFitMode
    });
    return previewInfo;
  }

  private priGetOrientedVideoSize(): DemoVideoSize {
    let width = this.config.videoConfig.width;
    let height = this.config.videoConfig.height;
    if (this.isLandscape) {
      const tmp = width;
      width = height;
      height = tmp;
    }
    return new DemoVideoSize(width, height);
  }

  private priGetPreviewSourceSize(): DemoPreviewSize {
    const width = this.config.videoConfig.width;
    const height = this.config.videoConfig.height;
    const realWidth = Math.max(width, height);
    const realHeight = Math.min(width, height);
    return new DemoPreviewSize(realWidth, realHeight);
  }

  private priBuildPusherConfig(): PusherConfig {
    const videoSize = this.priGetOrientedVideoSize();
    const videoConfig: VideoConfig = {
      codecID: this.config.videoConfig.codecID ?? VideoCodecType.HJVCodecH265,
      width: videoSize.width,
      height: videoSize.height,
      bitrate: this.config.videoConfig.bitrate,
      frameRate: this.config.videoConfig.frameRate,
      gopSize: this.config.videoConfig.gopSize,
      videoIsROIEnc: this.config.videoConfig.videoIsROIEnc
    };
    const audioConfig: AudioConfig = {
      codecID: this.config.audioConfig.codecID ?? AudioCodecType.HJCodecAAC,
      bitrate: this.config.audioConfig.bitrate,
      sampleFmt: this.config.audioConfig.sampleFmt,
      samplesRate: this.config.audioConfig.samplesRate,
      channels: this.config.audioConfig.channels
    };
    const pusherConfig: PusherConfig = {
      videoConfig: videoConfig,
      audioConfig: audioConfig,
      url: this.config.url
    };
    return pusherConfig;
  }

  private priSyncPreviewRotation(): void {
    if (!this.isPreviewOpen) {
      console.info('[HJPusherDemo] syncPreviewRotation skipped: preview not open');
      return;
    }
    const previewRotation = cameraService.updatePreviewRotation();
    console.info(`[HJPusherDemo] syncPreviewRotation rotation=${previewRotation}`);
    if (previewRotation !== undefined) {
      this.hjPusher.setPreviewRotation(previewRotation);
    }
  }

  private priUpdateStatus(message: string): void {
    this.currentStatus = message;
    console.info(`[HJPusherDemo] status=${message}`);
  }

  private priGetCurrentCameraPosition(): number {
    return this.isFrontCamera ? 1 : 0;
  }

  private priGetDemoOptionCodecId(): number {
    return this.pusherDemoOption.codecID;
  }

  private priGetDefaultEncodingUrl(): string {
    const currentUrl = (this.config.url ?? '').trim();
    if (currentUrl.length > 0) {
      return currentUrl;
    }
    return this.pusherDemoOption.url;
  }

  private priGetDefaultEncodingCodecId(): number {
    const currentUrl = (this.config.url ?? '').trim();
    if (currentUrl.length > 0) {
      return this.config.videoConfig.codecID ?? this.priGetDemoOptionCodecId();
    }
    return this.priGetDemoOptionCodecId();
  }

  private priGetDefaultFaceuUrl(): string {
    const configuredPath = (this.commonDemoOption.faceuPath ?? '').trim();
    if (!configuredPath) {
      return this.priJoinResourcePath('faceu/60031_10');
    }
    if (configuredPath.startsWith('/')) {
      return configuredPath;
    }
    const resourceDir = this.getUIContext().getHostContext()?.resourceDir ?? '';
    if (resourceDir.length > 0 && configuredPath.startsWith(resourceDir)) {
      return configuredPath;
    }
    return this.priJoinResourcePath(configuredPath);
  }

  private priGetDefaultPngGiftUrl(): string {
    const configuredPath = (this.commonDemoOption.pngGiftPath ?? '').trim();
    if (!configuredPath) {
      return this.priJoinResourcePath('pngseq/ShuangDanCaiShen');
    }
    if (configuredPath.startsWith('/')) {
      return configuredPath;
    }
    const resourceDir = this.getUIContext().getHostContext()?.resourceDir ?? '';
    if (resourceDir.length > 0 && configuredPath.startsWith(resourceDir)) {
      return configuredPath;
    }
    return this.priJoinResourcePath(configuredPath);
  }

  private priResolveFaceuUrl(inputPath: string): string {
    const trimmedPath = inputPath.trim();
    if (!trimmedPath) {
      return '';
    }
    if (trimmedPath.startsWith('/')) {
      return trimmedPath;
    }
    const resourceDir = this.getUIContext().getHostContext()?.resourceDir ?? '';
    if (resourceDir.length > 0 && trimmedPath.startsWith(resourceDir)) {
      return trimmedPath;
    }
    return this.priJoinResourcePath(trimmedPath);
  }

  private priResolvePngGiftUrl(inputPath: string): string {
    const trimmedPath = inputPath.trim();
    if (!trimmedPath) {
      return '';
    }
    if (trimmedPath.startsWith('/')) {
      return trimmedPath;
    }
    const resourceDir = this.getUIContext().getHostContext()?.resourceDir ?? '';
    if (resourceDir.length > 0 && trimmedPath.startsWith(resourceDir)) {
      return trimmedPath;
    }
    return this.priJoinResourcePath(trimmedPath);
  }

  private priGetDefaultRecordUrl(): string {
    const configuredUrl = (this.pusherDemoOption.recordUrl ?? '').trim();
    if (!configuredUrl) {
      return '/data/storage/el2/base/haps/entry/files/HJPusherDemoRecord.mp4';
    }
    return configuredUrl;
  }

  private priJoinResourcePath(relativePath: string): string {
    const normalizedPath = relativePath.replace(/^\/+/, '');
    const resourceDir = this.getUIContext().getHostContext()?.resourceDir ?? '';
    if (!resourceDir) {
      return normalizedPath;
    }
    if (resourceDir.endsWith('/')) {
      return `${resourceDir}${normalizedPath}`;
    }
    return `${resourceDir}/${normalizedPath}`;
  }

  private priShowToast(message: string): void {
    this.getUIContext().getPromptAction().showToast({ message });
  }

  private priGetRandomTestTypeByIndex(index: number): DemoRandomTestType {
    return this.randomTestTypes[index] ?? DemoRandomTestType.Off;
  }

  private priGetCurrentRandomTestType(): DemoRandomTestType {
    return this.priGetRandomTestTypeByIndex(this.randomTestSelectedIndex);
  }

  private priGetCurrentRandomTestLabel(): string {
    return this.randomTestOptions[this.randomTestSelectedIndex]?.value ?? '关闭随机';
  }

  private priGetRandomTestDelayMs(): number {
    const minDelayMs: number = 20;
    const maxDelayMs: number = 2000;
    return minDelayMs + Math.floor(Math.random() * (maxDelayMs - minDelayMs + 1));
  }

  private priClearRenderSurfaceRebuildTimer(): void {
    if (this.renderSurfaceRebuildTimer >= 0) {
      clearTimeout(this.renderSurfaceRebuildTimer);
      this.renderSurfaceRebuildTimer = -1;
    }
  }

  private priClearRandomTestTimer(): void {
    if (this.randomTestTimer >= 0) {
      clearTimeout(this.randomTestTimer);
      this.randomTestTimer = -1;
    }
  }

  private priUnmountRenderSurface(): void {
    this.priClearRenderSurfaceRebuildTimer();
    this.isRenderSurfaceMounted = false;
    console.info(`[HJPusherDemo] unmountRenderSurface version=${this.renderSurfaceVersion}`);
  }

  private async priRebuildRenderSurface(): Promise<void> {
    this.priUnmountRenderSurface();
    return new Promise<void>((resolve: () => void) => {
      this.renderSurfaceRebuildTimer = setTimeout(() => {
        this.renderSurfaceRebuildTimer = -1;
        this.renderSurfaceVersion += 1;
        this.isRenderSurfaceMounted = true;
        console.info(`[HJPusherDemo] rebuildRenderSurface mounted version=${this.renderSurfaceVersion}`);
        resolve();
      }, 0) as number;
    });
  }

  private async priTogglePreview(): Promise<void> {
    console.info(`[HJPusherDemo] togglePreview isPreviewOpen=${this.isPreviewOpen}`);
    if (this.isPreviewOpen) {
      await this.priClosePreviewFlow();
      return;
    }
    await this.priOpenPreviewFlow();
  }

  private async priOpenPreviewFlow(): Promise<void> {
    this.priUpdateStatus('正在开启预览...');
    if (!this.priHasRequiredPermissions()) {
      const granted = await this.priEnsurePermissionsOrToast();
      if (!granted) {
        return;
      }
    }
    const previewInfo = this.priBuildPreviewInfo();
    console.info(`[HJPusherDemo] openPreviewFlow previewInfo=${JSON.stringify(previewInfo)} isLandscape=${this.isLandscape}`);
    const previewSurfaceId = this.hjPusher.openPreview(previewInfo, (str: string) => {
      this.priUpdateStatus(`预览回调: ${str}`);
    });
    this.isPreviewOpen = true;
    console.info(`[HJPusherDemo] openPreviewFlow previewSurfaceId=${previewSurfaceId.toString()}`);
    if (previewSurfaceId === 0n) {
      console.error('[HJPusherDemo] openPreviewFlow previewSurfaceId is 0');
      this.priUpdateStatus('预览开启失败');
      this.priShowToast('预览开启失败');
      return;
    }

    try {
      console.info('[HJPusherDemo] openPreviewFlow initCamera');
      cameraService.initCamera(getContext() as Context);
      console.info(`[HJPusherDemo] openPreviewFlow bindSurfaceId=${previewSurfaceId.toString()}`);
      cameraService.bindSurfaceId(previewSurfaceId.toString());
      console.info(`[HJPusherDemo] openPreviewFlow startPreview camera=${this.priGetCurrentCameraPosition()} isFrontCamera=${this.isFrontCamera}`);
      await cameraService.startPreview(this.priGetCurrentCameraPosition());
      this.priSyncPreviewRotation();
      console.info('[HJPusherDemo] openPreviewFlow startPreview done');

      console.info('[HJPusherDemo] openPreviewFlow notifyPreviewOpened');
      this.controller.notifyPreviewOpened();
      this.priUpdateStatus('预览已开启');

    } catch (error) {
      console.error(`[HJPusherDemo] openPreviewFlow failed: ${JSON.stringify(error)}`);
      await this.priCloseFaceDetectForPreview();
      await cameraService.releaseCamera();
      this.hjPusher.closePreview();
      this.priUpdateStatus('预览开启失败');
      this.priShowToast('预览开启失败');
    }
  }

  private async priClosePreviewFlow(rebuildSurface: boolean = true): Promise<void> {
    if (!this.isPreviewOpen) {
      console.info('[HJPusherDemo] closePreviewFlow skipped: preview not open');
      return;
    }
    this.priUpdateStatus('正在关闭预览...');
    this.priClosePreviewRelatedState();
    this.priCloseFaceuFlow();
    await this.priCloseFaceDetectForPreview();
    try {
      console.info('[HJPusherDemo] closePreviewFlow releaseCamera');
      await cameraService.releaseCamera();
    } catch (error) {
      console.error(`[HJPusherDemo] releaseCamera failed: ${JSON.stringify(error)}`);
    }
    console.info('[HJPusherDemo] closePreviewFlow closePreview');
    this.hjPusher.closePreview();
    console.info('[HJPusherDemo] closePreviewFlow notifyPreviewClosed');
    this.controller.notifyPreviewClosed();
    this.isPreviewOpen = false;
    if (rebuildSurface) {
      await this.priRebuildRenderSurface();
    }
    this.priUpdateStatus('预览已关闭');
  }

  private priClosePreviewRelatedState(): void {
    console.info(`[HJPusherDemo] closePreviewRelatedState encodingOpen=${this.isEncodingOpen} recorderOpen=${this.isRecorderOpen} settingsOpen=${this.showSettingsDialog}`);
    this.priCloseRecorderFlow();
    this.priCloseEncodingFlow();
    this.priCloseSettingsDialog();
    this.priCloseFaceuDialog();
  }

  private priEnsureFaceDetectCallbacksRegistered(): void {
    if (this.faceDetectRegistered) {
      return;
    }
    this.faceDetectMgr.registerFaceInfoCb((width: number, height: number, faceInfo: string) => {
      // if (faceInfo && faceInfo.trim().length > 0) {
      //   console.info(`[HJPusherDemo] faceInfo width=${width} height=${height} faceInfo=${faceInfo}`);
      // }
      this.hjPusher.setFaceInfo(HJRteGraphConfig.HJNodeClass_SourceBridge, width, height, faceInfo);
    });
    this.faceDetectMgr.registerNativeSourceOpenCb((): number => {
      return this.hjPusher.nativeSourceOpen(true);
    });
    this.faceDetectMgr.registerNativeSourceAcquireCb((): HJNativeSourceData | null => {
      return this.hjPusher.nativeSourceAcquire();
    });
    this.faceDetectMgr.registerNativeSourceCloseCb((): void => {
      this.hjPusher.nativeSourceClose();
    });
    this.faceDetectRegistered = true;
  }

  private async priOpenFaceDetectForPreview(): Promise<void> {
    if (!this.isPreviewOpen || this.isFaceDetectOpen) {
      return;
    }
    this.priEnsureFaceDetectCallbacksRegistered();
    try {
      this.faceDetectRet = await this.faceDetectMgr.openFaceDetect();
      if (this.faceDetectRet !== HJFaceDetectMgrRet.ok) {
        console.error(`[HJPusherDemo] openFaceDetect unexpected ret=${this.faceDetectRet}`);
        await this.priCleanupFaceDetectAfterFailure();
        this.priShowToast('人脸检测开启失败');
        return;
      }
      this.isFaceDetectOpen = true;
      console.info('[HJPusherDemo] openFaceDetect success');
    } catch (error) {
      console.error(`[HJPusherDemo] openFaceDetect failed: ${JSON.stringify(error)}`);
      this.isFaceDetectOpen = false;
      await this.priCleanupFaceDetectAfterFailure();
      this.priShowToast('人脸检测开启失败');
    }
  }

  private async priCloseFaceDetectForPreview(): Promise<void> {
    if (!this.isFaceDetectOpen) {
      return;
    }
    try {
      this.faceDetectRet = await this.faceDetectMgr.closeFaceDetect();
      console.info(`[HJPusherDemo] closeFaceDetect ret=${this.faceDetectRet}`);
    } catch (error) {
      console.error(`[HJPusherDemo] closeFaceDetect failed: ${JSON.stringify(error)}`);
    } finally {
      this.isFaceDetectOpen = false;
    }
  }

  private async priCleanupFaceDetectAfterFailure(): Promise<void> {
    try {
      await this.faceDetectMgr.closeFaceDetect();
    } catch (error) {
      console.error(`[HJPusherDemo] cleanupFaceDetectAfterFailure failed: ${JSON.stringify(error)}`);
    } finally {
      this.isFaceDetectOpen = false;
    }
  }

  private async priEnsureRandomPreviewOpened(): Promise<boolean> {
    if (this.isPreviewOpen) {
      return true;
    }
    await this.priOpenPreviewFlow();
    return this.isPreviewOpen;
  }

  private async priEnsureRandomFaceDetectOpened(): Promise<boolean> {
    const previewOpened = await this.priEnsureRandomPreviewOpened();
    if (!previewOpened) {
      return false;
    }
    if (!this.isFaceDetectOpen) {
      await this.priOpenFaceDetectForPreview();
    }
    return this.isFaceDetectOpen;
  }

  private async priExecuteRandomPreviewAction(): Promise<void> {
    if (this.isPreviewOpen) {
      await this.priClosePreviewFlow();
      return;
    }
    await this.priOpenPreviewFlow();
  }

  private async priExecuteRandomEncodingAction(): Promise<void> {
    if (this.isEncodingOpen) {
      this.priCloseEncodingFlow();
      return;
    }
    const faceDetectOpened = await this.priEnsureRandomFaceDetectOpened();
    if (!faceDetectOpened) {
      return;
    }
    if (!this.config.url || !this.config.url.trim()) {
      this.config.url = this.priGetDefaultEncodingUrl();
    }
    this.priOpenEncodingFlow();
  }

  private async priExecuteRandomFaceuAction(): Promise<void> {
    if (this.isFaceuOpen) {
      this.priCloseFaceuFlow();
      return;
    }
    const faceDetectOpened = await this.priEnsureRandomFaceDetectOpened();
    if (!faceDetectOpened) {
      return;
    }
    this.faceuUrlDraft = this.priGetDefaultFaceuUrl();
    this.priOpenFaceuFlow();
  }

  private async priExecuteRandomFaceDetectAction(): Promise<void> {
    if (this.isFaceDetectOpen) {
      if (this.isFaceuOpen) {
        this.priCloseFaceuFlow();
      }
      if (this.isFaceProtectedOpen) {
        this.hjPusher.setFaceProtected(false);
        this.isFaceProtectedOpen = false;
      }
      await this.priCloseFaceDetectForPreview();
      return;
    }
    await this.priEnsureRandomFaceDetectOpened();
  }

  private async priExecuteRandomTestAction(): Promise<void> {
    let actionType: DemoRandomTestType = this.priGetCurrentRandomTestType();
    if (actionType === DemoRandomTestType.All) {
      const randomActionTypes: DemoRandomTestType[] = [
        DemoRandomTestType.Preview,
        DemoRandomTestType.Encoding,
        DemoRandomTestType.Faceu,
        DemoRandomTestType.FaceDetect,
        DemoRandomTestType.Window,
        DemoRandomTestType.Mode
      ];
      const randomIndex = Math.floor(Math.random() * randomActionTypes.length);
      actionType = randomActionTypes[randomIndex];
    }
    console.info(`[HJPusherDemo] executeRandomTestAction type=${actionType}`);
    if (actionType === DemoRandomTestType.Preview) {
      await this.priExecuteRandomPreviewAction();
      return;
    }
    if (actionType === DemoRandomTestType.Encoding) {
      await this.priExecuteRandomEncodingAction();
      return;
    }
    if (actionType === DemoRandomTestType.Faceu) {
      await this.priExecuteRandomFaceuAction();
      return;
    }
    if (actionType === DemoRandomTestType.FaceDetect) {
      await this.priExecuteRandomFaceDetectAction();
      return;
    }
    if (actionType === DemoRandomTestType.Window) {
      this.priToggleRenderWindowSize();
      return;
    }
    if (actionType === DemoRandomTestType.Mode) {
      this.priTogglePreviewDisplayMode();
    }
  }

  private priScheduleRandomTest(): void {
    if (!this.isRandomTestRunning) {
      return;
    }
    const delayMs: number = this.priGetRandomTestDelayMs();
    this.priUpdateStatus(`随机测试等待 ${delayMs}ms: ${this.priGetCurrentRandomTestLabel()}`);
    this.randomTestTimer = setTimeout(() => {
      this.randomTestTimer = -1;
      void this.priRunRandomTestAction();
    }, delayMs) as number;
  }

  private async priRunRandomTestAction(): Promise<void> {
    if (!this.isRandomTestRunning || this.randomTestActionInFlight) {
      return;
    }
    this.randomTestActionInFlight = true;
    try {
      await this.priExecuteRandomTestAction();
    } finally {
      this.randomTestActionInFlight = false;
      if (this.isRandomTestRunning) {
        this.priScheduleRandomTest();
      }
    }
  }

  private async priStopRandomTest(updateStatus: boolean = true): Promise<void> {
    const wasRunning = this.isRandomTestRunning;
    this.priClearRandomTestTimer();
    this.isRandomTestRunning = false;
    if (this.isFaceuOpen) {
      this.priCloseFaceuFlow();
    }
    if (this.isFaceProtectedOpen) {
      this.hjPusher.setFaceProtected(false);
      this.isFaceProtectedOpen = false;
    }
    if (this.isFaceDetectOpen) {
      await this.priCloseFaceDetectForPreview();
    }
    if (updateStatus && wasRunning) {
      this.priUpdateStatus('随机测试已关闭');
    }
  }

  private async priStartRandomTest(): Promise<void> {
    await this.priStopRandomTest(false);
    this.isRandomTestRunning = true;
    await this.priEnsureRandomFaceDetectOpened();
    this.priUpdateStatus(`随机测试已开启: ${this.priGetCurrentRandomTestLabel()}`);
    this.priScheduleRandomTest();
  }

  private priHandleRandomTestSelect(index: number): void {
    this.randomTestSelectedIndex = index;
    const randomTestType = this.priGetCurrentRandomTestType();
    console.info(`[HJPusherDemo] handleRandomTestSelect index=${index} type=${randomTestType}`);
    if (randomTestType === DemoRandomTestType.Off) {
      void this.priStopRandomTest(true);
      return;
    }
    void this.priStartRandomTest();
  }

  private priToggleEncoding(): void {
    console.info(`[HJPusherDemo] toggleEncoding isEncodingOpen=${this.isEncodingOpen}`);
    if (this.isEncodingOpen) {
      this.priCloseEncodingFlow();
      return;
    }
    if (!this.isPreviewOpen) {
      console.info('[HJPusherDemo] toggleEncoding blocked: preview not open');
      this.priShowToast('推流失败，请先开启预览');
      return;
    }
    this.priOpenSettingsDialog(true);
  }

  private priToggleFaceu(): void {
    console.info(`[HJPusherDemo] toggleFaceu isFaceuOpen=${this.isFaceuOpen} previewOpen=${this.isPreviewOpen} faceDetectOpen=${this.isFaceDetectOpen}`);
    if (this.isFaceuOpen) {
      this.priCloseFaceuFlow();
      return;
    }
    if (!this.isPreviewOpen) {
      this.priShowToast('请先开启预览');
      return;
    }
    if (!this.isFaceDetectOpen) {
      this.priShowToast('人脸检测未就绪');
      return;
    }
    this.priOpenFaceuDialog();
  }

  private priTogglePngGift(): void {
    console.info(`[HJPusherDemo] togglePngGift previewOpen=${this.isPreviewOpen}`);
    this.priOpenPngGiftDialog();
  }

  private priToggleFaceProtected(): void {
    console.info(`[HJPusherDemo] toggleFaceProtected isFaceProtectedOpen=${this.isFaceProtectedOpen} faceDetectOpen=${this.isFaceDetectOpen}`);
    if (!this.isFaceDetectOpen) {
      this.priShowToast('开启人脸保护失败，请先开启检测');
      return;
    }
    const nextFaceProtected = !this.isFaceProtectedOpen;
    this.hjPusher.setFaceProtected(nextFaceProtected);
    this.isFaceProtectedOpen = nextFaceProtected;
    this.priUpdateStatus(nextFaceProtected ? '人脸保护已开启' : '人脸保护已关闭');
  }

  private async priToggleFaceDetect(): Promise<void> {
    console.info(`[HJPusherDemo] toggleFaceDetect isFaceDetectOpen=${this.isFaceDetectOpen} previewOpen=${this.isPreviewOpen} faceuOpen=${this.isFaceuOpen}`);
    if (this.isFaceDetectOpen) {
      if (this.isFaceuOpen) {
        this.priCloseFaceuFlow();
      }
      if (this.isFaceProtectedOpen) {
        this.hjPusher.setFaceProtected(false);
        this.isFaceProtectedOpen = false;
      }
      await this.priCloseFaceDetectForPreview();
      this.priUpdateStatus('检测已关闭');
      return;
    }
    if (!this.isPreviewOpen) {
      this.priShowToast('请先开启预览');
      return;
    }
    await this.priOpenFaceDetectForPreview();
    if (this.isFaceDetectOpen) {
      this.priUpdateStatus('检测已开启');
    }
  }

  private priToggleRecorder(): void {
    console.info(`[HJPusherDemo] toggleRecorder isRecorderOpen=${this.isRecorderOpen} previewOpen=${this.isPreviewOpen}`);
    if (this.isRecorderOpen) {
      this.priCloseRecorderFlow();
      return;
    }
    this.priOpenRecorderFlow();
  }

  private priHandlePreviewButtonClick(): void {
    console.info('[HJPusherDemo] handlePreviewButtonClick');
    void this.priTogglePreview();
  }

  private priHandleFaceDetectButtonClick(): void {
    void this.priToggleFaceDetect();
  }

  private priHandleCameraButtonClick(): void {
    void this.priToggleCameraFacing();
  }

  private priToggleRenderWindowSize(): void {
    if ((this.windowSizeSwitchIndex % 4) === 0) {
      this.xComponentWidth = $r('app.float.xcomponent_width2');
      this.xComponentHeight = $r('app.float.xcomponent_height2');
    } else if ((this.windowSizeSwitchIndex % 4) === 1) {
      this.xComponentWidth = $r('app.float.xcomponent_width3');
      this.xComponentHeight = $r('app.float.xcomponent_height3');
    } else if ((this.windowSizeSwitchIndex % 4) === 2) {
      this.xComponentWidth = $r('app.float.xcomponent_width4');
      this.xComponentHeight = $r('app.float.xcomponent_height4');
    } else {
      this.xComponentWidth = $r('app.float.xcomponent_width');
      this.xComponentHeight = $r('app.float.xcomponent_height');
    }
    this.windowSizeSwitchIndex++;
    console.info(`[HJPusherDemo] toggleRenderWindowSize index=${this.windowSizeSwitchIndex} previewOpen=${this.isPreviewOpen}`);
    this.priUpdateStatus('预览窗口尺寸已切换');
  }

  private priBuildPreviewDisplayModeLinkInfo(): string {
    const linkInfo = new HJRteJsonLinkInfo();
    linkInfo.setLink(this.isPreviewFitMode ? HJRteGraphConfig.HJNodeRenderType_Fit : HJRteGraphConfig.HJNodeRenderType_Clip);
    return JSON.stringify(linkInfo);
  }

  private priApplyPreviewDisplayMode(): boolean {
    if (!this.isPreviewOpen) {
      console.info(`[HJPusherDemo] applyPreviewDisplayMode skipped previewOpen=${this.isPreviewOpen}`);
      return true;
    }
    const renderMode = this.isPreviewFitMode ? HJRteGraphConfig.HJNodeRenderType_Fit : HJRteGraphConfig.HJNodeRenderType_Clip;
    const linkInfo = this.priBuildPreviewDisplayModeLinkInfo();
    const sourceNodes: HJRteAdjacentNodeInfo[] = this.hjPusher.nodeGetPre(
      HJRteGraphConfig.HJNodeClass_TargetUI_0,
      HJRteGraphConfig.HJNodeClass_TargetUI_0
    );
    let applied = false;
    for (const sourceNode of sourceNodes) {
      const ret = this.hjPusher.nodeLinkInfoChange(
        sourceNode.classStyle,
        sourceNode.insName,
        HJRteGraphConfig.HJNodeClass_TargetUI_0,
        HJRteGraphConfig.HJNodeClass_TargetUI_0,
        linkInfo
      );
      console.info(`[HJPusherDemo] applyPreviewDisplayMode source=${sourceNode.classStyle}:${sourceNode.insName} renderMode=${renderMode} ret=${ret}`);
      if (ret >= 0) {
        applied = true;
      }
    }
    return applied;
  }

  private priTogglePreviewDisplayMode(): void {
    const nextFitMode = !this.isPreviewFitMode;
    this.isPreviewFitMode = nextFitMode;
    const modeText = nextFitMode ? '显示填充' : '显示裁剪';
    console.info(`[HJPusherDemo] togglePreviewDisplayMode mode=${modeText} previewOpen=${this.isPreviewOpen}`);
    if (this.isPreviewOpen && !this.priApplyPreviewDisplayMode()) {
      this.isPreviewFitMode = !nextFitMode;
      this.priShowToast('显示模式切换失败');
      return;
    }
    if (this.isPreviewOpen) {
      this.priUpdateStatus(`显示模式已切为 ${modeText}`);
    } else {
      this.priUpdateStatus(`显示模式已设为 ${modeText}，下次预览生效`);
    }
  }

  private priOpenRecorderFlow(): void {
    if (!this.isPreviewOpen) {
      console.info('[HJPusherDemo] openRecorderFlow blocked: preview not open');
      this.priShowToast('录制失败，请先开启预览');
      return;
    }
    if (!this.isEncodingOpen) {
      console.info('[HJPusherDemo] openRecorderFlow blocked: encoding not open');
      this.priShowToast('录制失败，请先开启推流，才可录制');
      return;
    }
    const recordUrl = this.priGetDefaultRecordUrl();
    if (!recordUrl) {
      console.info('[HJPusherDemo] openRecorderFlow blocked: record url empty');
      this.priShowToast('录制路径不能为空');
      return;
    }
    const recorderInfo = new RecorderInfo();
    recorderInfo.recordUrl = recordUrl;
    const ret = this.hjPusher.openRecorder(recorderInfo);
    console.info(`[HJPusherDemo] openRecorderFlow ret=${ret} recordUrl=${recordUrl}`);
    if (ret < 0) {
      this.priShowToast('录制开启失败');
      return;
    }
    this.isRecorderOpen = true;
    this.priUpdateStatus('录制已开启');
  }

  private priCloseRecorderFlow(): void {
    if (!this.isRecorderOpen) {
      console.info('[HJPusherDemo] closeRecorderFlow skipped: recorder not open');
      return;
    }
    console.info('[HJPusherDemo] closeRecorderFlow');
    this.hjPusher.closeRecorder();
    this.isRecorderOpen = false;
    this.priUpdateStatus('录制已关闭');
  }

  private priOpenFaceuDialog(): void {
    this.faceuUrlDraft = this.priGetDefaultFaceuUrl();
    this.showFaceuDialog = true;
    console.info(`[HJPusherDemo] openFaceuDialog faceuUrlDraft=${this.faceuUrlDraft}`);
  }

  private priCloseFaceuDialog(): void {
    this.showFaceuDialog = false;
    console.info('[HJPusherDemo] closeFaceuDialog');
  }

  private priOpenPngGiftDialog(): void {
    this.pngGiftUrlDraft = this.priGetDefaultPngGiftUrl();
    this.showPngGiftDialog = true;
    console.info(`[HJPusherDemo] openPngGiftDialog pngGiftUrlDraft=${this.pngGiftUrlDraft}`);
  }

  private priClosePngGiftDialog(): void {
    this.showPngGiftDialog = false;
    console.info('[HJPusherDemo] closePngGiftDialog');
  }

  private priOpenFaceuFlow(): void {
    const nextFaceuUrl = this.priResolveFaceuUrl(this.faceuUrlDraft);
    if (!nextFaceuUrl) {
      this.priShowToast('脸萌路径不能为空');
      return;
    }
    const ret = this.hjPusher.nodeEnable(
      HJRteGraphConfig.HJNodeClass_SourceFaceu,
      HJRteGraphConfig.HJNodeClass_SourceFaceu,
      true,
      nextFaceuUrl
    );
    console.info(`[HJPusherDemo] openFaceuFlow ret=${ret} url=${nextFaceuUrl}`);
    if (ret < 0) {
      this.priShowToast('脸萌开启失败');
      return;
    }
    this.commonDemoOption.faceuPath = nextFaceuUrl;
    this.isFaceuOpen = true;
    this.showFaceuDialog = false;
    if (this.isPreviewOpen) {
      const displayModeApplied = this.priApplyPreviewDisplayMode();
      console.info(`[HJPusherDemo] openFaceuFlow applyPreviewDisplayMode=${displayModeApplied} fitMode=${this.isPreviewFitMode}`);
    }
    this.priUpdateStatus('脸萌已开启');
  }

  private priOpenPngGiftFlow(): void {
    if (!this.isPreviewOpen) {
      this.priShowToast('请先开启预览');
      return;
    }
    const nextPngGiftUrl = this.priResolvePngGiftUrl(this.pngGiftUrlDraft);
    if (!nextPngGiftUrl) {
      this.priShowToast('礼物 PNG 路径不能为空');
      return;
    }
    console.info(`[HJPusherDemo] openPngGiftFlow url=${nextPngGiftUrl}`);
    this.hjPusher.openPngSeq(nextPngGiftUrl);
    this.commonDemoOption.pngGiftPath = nextPngGiftUrl;
    this.showPngGiftDialog = false;
    this.priUpdateStatus('礼物 PNG 已打开');
  }

  private priCloseFaceuFlow(): void {
    if (!this.isFaceuOpen) {
      return;
    }
    const ret = this.hjPusher.nodeEnable(
      HJRteGraphConfig.HJNodeClass_SourceFaceu,
      HJRteGraphConfig.HJNodeClass_SourceFaceu,
      false,
      ''
    );
    console.info(`[HJPusherDemo] closeFaceuFlow ret=${ret}`);
    this.isFaceuOpen = false;
    if (this.isPreviewOpen) {
      const displayModeApplied = this.priApplyPreviewDisplayMode();
      console.info(`[HJPusherDemo] closeFaceuFlow applyPreviewDisplayMode=${displayModeApplied} fitMode=${this.isPreviewFitMode}`);
    }
    this.priUpdateStatus('脸萌已关闭');
  }

  private async priToggleCameraFacing(): Promise<void> {
    const nextIsFrontCamera = !this.isFrontCamera;
    console.info(`[HJPusherDemo] toggleCameraFacing nextIsFrontCamera=${nextIsFrontCamera} previewOpen=${this.isPreviewOpen}`);
    if (!this.isPreviewOpen) {
      this.isFrontCamera = nextIsFrontCamera;
      this.priUpdateStatus(this.isFrontCamera ? '已切换到前置摄像头' : '已切换到后置摄像头');
      return;
    }
    try {
      console.info('[HJPusherDemo] toggleCameraFacing restart preview begin');
      await this.priClosePreviewFlow(true);
      this.isFrontCamera = nextIsFrontCamera;
      await this.priOpenPreviewFlow();
      if (this.isPreviewOpen) {
        this.priUpdateStatus(this.isFrontCamera ? '已切换到前置摄像头并重启预览' : '已切换到后置摄像头并重启预览');
      }
    } catch (error) {
      console.error(`[HJPusherDemo] toggleCameraFacing failed: ${JSON.stringify(error)}`);
      this.priShowToast('切换摄像头失败');
    }
  }

  private priCanOpenEncoding(): boolean {
    const isROIEncodingEnabled = this.config.videoConfig.videoIsROIEnc ?? false;
    if (isROIEncodingEnabled && !this.isFaceDetectOpen) {
      console.info('[HJPusherDemo] canOpenEncoding blocked: ROI encoding requires face detect');
      this.priShowToast('ROI 功能需要先打开检测');
      return false;
    }
    return true;
  }

  private priOpenEncodingFlow(): void {
    if (!this.isPreviewOpen) {
      console.info('[HJPusherDemo] openEncodingFlow blocked: preview not open');
      this.priShowToast('请先开启预览');
      return;
    }
    if (!this.config.url || !this.config.url.trim()) {
      console.info('[HJPusherDemo] openEncodingFlow blocked: url empty');
      this.priShowToast('请先在设置中填写 RTMP 地址');
      return;
    }
    if (!this.priCanOpenEncoding()) {
      return;
    }

    const pusherConfig: PusherConfig = this.priBuildPusherConfig();
    const stateInfo: MediaStateInfo = {
      uid: 2342,
      device: 'Harmony',
      sn: 'HJPusherDemo'
    };
    console.info(`[HJPusherDemo] openEncodingFlow config=${JSON.stringify(pusherConfig)} stateInfo=${JSON.stringify(stateInfo)}`);
    try {
      this.hjPusher.setVideoEncQuantOffset(this.config.roiQuantOffset);
      this.hjPusher.openPusher(pusherConfig, stateInfo, (str: string) => {
        this.priUpdateStatus(`推流回调: ${str}`);
      });
      this.isEncodingOpen = true;
      this.priUpdateStatus('推流已开启');
    } catch (error) {
      console.error(`[HJPusherDemo] openEncodingFlow failed: ${JSON.stringify(error)}`);
      this.priShowToast('推流开启失败');
    }
  }

  private priCloseEncodingFlow(): void {
    if (!this.isEncodingOpen) {
      console.info('[HJPusherDemo] closeEncodingFlow skipped: encoding not open');
      return;
    }
    console.info('[HJPusherDemo] closeEncodingFlow');
    this.hjPusher.closePusher();
    this.isEncodingOpen = false;
    this.priUpdateStatus('推流已关闭');
  }

  private priToggleOrientation(): void {
    if (this.isPreviewOpen) {
      console.info('[HJPusherDemo] toggleOrientation ignored: preview is open');
      return;
    }
    this.isLandscape = !this.isLandscape;
    console.info(`[HJPusherDemo] toggleOrientation isLandscape=${this.isLandscape}`);
    WindowUtils.setOrientation(this.isLandscape ? window.Orientation.LANDSCAPE : window.Orientation.PORTRAIT);
    this.priUpdateStatus(this.isLandscape ? '切换到横屏' : '切换到竖屏');
  }

  private priOpenSettingsDialog(startEncodingAfterSave: boolean = false): void {
    this.urlDraft = this.priGetDefaultEncodingUrl();
    this.roiEnabledDraft = this.config.videoConfig.videoIsROIEnc ?? false;
    this.roiOffsetDraft = this.config.roiQuantOffset;
    this.videoCodecDraft = this.priGetDefaultEncodingCodecId();
    this.pendingEncodingStart = startEncodingAfterSave;
    this.showSettingsDialog = true;
    console.info(`[HJPusherDemo] openSettingsDialog urlDraft=${this.urlDraft} roi=${this.roiEnabledDraft} roiOffset=${this.roiOffsetDraft} codecID=${this.videoCodecDraft} startEncodingAfterSave=${startEncodingAfterSave}`);
  }

  private priCloseSettingsDialog(): void {
    this.showSettingsDialog = false;
    this.pendingEncodingStart = false;
    console.info('[HJPusherDemo] closeSettingsDialog');
  }

  private priSaveSettings(): void {
    const nextUrl = this.urlDraft.trim();
    if (!nextUrl) {
      console.info('[HJPusherDemo] saveSettings blocked: empty url');
      this.priShowToast('RTMP 地址不能为空');
      return;
    }
    const shouldStartEncoding = this.pendingEncodingStart && !this.isEncodingOpen;
    if (shouldStartEncoding && this.roiEnabledDraft && !this.isFaceDetectOpen) {
      console.info('[HJPusherDemo] saveSettings blocked: ROI requires face detect before start push');
      this.priShowToast('推流失败，ROI必须要求打开检测');
      return;
    }
    this.config.url = nextUrl;
    this.config.videoConfig.videoIsROIEnc = this.roiEnabledDraft;
    this.config.roiQuantOffset = this.roiOffsetDraft;
    this.config.videoConfig.codecID = this.videoCodecDraft;
    this.showSettingsDialog = false;
    this.pendingEncodingStart = false;
    console.info(`[HJPusherDemo] saveSettings url=${nextUrl} roi=${this.roiEnabledDraft} roiOffset=${this.roiOffsetDraft} codecID=${this.videoCodecDraft} startEncoding=${shouldStartEncoding}`);
    if (shouldStartEncoding) {
      this.priOpenEncodingFlow();
      return;
    }
    if (this.isEncodingOpen) {
      this.priShowToast('RTMP 地址已保存，下次开启推流生效');
    } else {
      this.priShowToast('RTMP 地址已保存');
    }
    this.priUpdateStatus('RTMP 地址已更新');
  }

  private async priCleanup(): Promise<void> {
    console.info('[HJPusherDemo] cleanup begin');
    await this.priStopRandomTest(false);
    this.priCloseSettingsDialog();
    this.priCloseFaceuDialog();
    this.priClosePngGiftDialog();
    this.priCloseFaceuFlow();
    this.priCloseEncodingFlow();
    if (this.isFaceProtectedOpen) {
      this.hjPusher.setFaceProtected(false);
      this.isFaceProtectedOpen = false;
    }
    this.priClearRenderSurfaceRebuildTimer();
    await this.priCloseFaceDetectForPreview();
    if (this.isPreviewOpen) {
      await this.priClosePreviewFlow(false);
    }
    this.priUnmountRenderSurface();
    this.hjPusher.destroyPusher();
    console.info('[HJPusherDemo] cleanup end');
  }

  private priHandleAction(actionType: DemoActionType): void {
    console.info(`[HJPusherDemo] handleAction actionType=${actionType}`);
    if (actionType === DemoActionType.Preview) {
      this.priHandlePreviewButtonClick();
      return;
    }
    if (actionType === DemoActionType.Encoding) {
      this.priToggleEncoding();
      return;
    }
    if (actionType === DemoActionType.Record) {
      this.priToggleRecorder();
      return;
    }
    if (actionType === DemoActionType.Camera) {
      this.priHandleCameraButtonClick();
      return;
    }
    if (actionType === DemoActionType.FaceDetect) {
      this.priHandleFaceDetectButtonClick();
      return;
    }
    if (actionType === DemoActionType.Faceu) {
      this.priToggleFaceu();
      return;
    }
    if (actionType === DemoActionType.GiftPng) {
      this.priTogglePngGift();
      return;
    }
    if (actionType === DemoActionType.FaceProtected) {
      this.priToggleFaceProtected();
      return;
    }
  }

  @Builder
  private priActionButton(label: string, actionType: DemoActionType) {
    Button(label)
      .fontSize($r('app.float.button_font_size'))
      .fontWeight(Common.FONT_WEIGHT_500)
      .height(36)
      .padding({ left: 16, right: 16 })
      .backgroundColor($r('app.color.button_clickable'))
      .fontColor(Color.White)
      .borderRadius(18)
      .onClick(() => {
        this.priHandleAction(actionType);
      });
  }

  @Builder
  private priOrientationButton() {
    if (this.isPreviewOpen) {
      Button(this.isLandscape ? '切为竖屏' : '切为横屏')
        .fontSize($r('app.float.button_font_size'))
        .fontWeight(Common.FONT_WEIGHT_500)
        .height(36)
        .padding({ left: 16, right: 16 })
        .backgroundColor('#335A6B7A')
        .fontColor(Color.White)
        .enabled(false)
        .borderRadius(18)
        .opacity(0.45);
    } else {
      Button(this.isLandscape ? '切为竖屏' : '切为横屏')
        .fontSize($r('app.float.button_font_size'))
        .fontWeight(Common.FONT_WEIGHT_500)
        .height(36)
        .padding({ left: 16, right: 16 })
        .backgroundColor($r('app.color.button_clickable'))
        .fontColor(Color.White)
        .borderRadius(18)
        .onClick(() => {
          if (!this.isPreviewOpen) {
            this.priToggleOrientation();
          }
        })
        .opacity(1);
    }
  }

  @Builder
  private priWindowSizeButton() {
    Button('切换窗口')
      .fontSize($r('app.float.button_font_size'))
      .fontWeight(Common.FONT_WEIGHT_500)
      .height(36)
      .padding({ left: 16, right: 16 })
      .backgroundColor($r('app.color.button_clickable'))
      .fontColor(Color.White)
      .borderRadius(18)
      .onClick(() => {
        this.priToggleRenderWindowSize();
      });
  }

  @Builder
  private priDisplayModeButton() {
    Button(this.isPreviewFitMode ? '显示裁剪' : '显示填充')
      .fontSize($r('app.float.button_font_size'))
      .fontWeight(Common.FONT_WEIGHT_500)
      .height(36)
      .padding({ left: 16, right: 16 })
      .backgroundColor($r('app.color.button_clickable'))
      .fontColor(Color.White)
      .borderRadius(18)
      .onClick(() => {
        this.priTogglePreviewDisplayMode();
      });
  }

  @Builder
  private priRandomTestSelect() {
    Select(this.randomTestOptions)
      .selected(this.randomTestSelectedIndex)
      .value(this.randomTestOptions[this.randomTestSelectedIndex]?.value ?? '关闭随机')
      .backgroundColor($r('app.color.button_clickable'))
      .height(36)
      .padding({ left: 12, right: 12 })
      .borderRadius(18)
      .onSelect((index: number) => {
        this.priHandleRandomTestSelect(index);
      });
  }

  @Builder
  private priSettingsDialog() {
    Column() {
      Blank()
        .width(Common.FULL_PERCENT)
        .layoutWeight(1)
        .onClick(() => {
          this.priCloseSettingsDialog();
        });

      Column({ space: 16 }) {
        Text('RTMP 推流设置')
          .fontSize(20)
          .fontWeight(Common.FONT_WEIGHT_500)
          .fontColor($r('app.color.status_font_color'))

        TextInput({ text: this.urlDraft, placeholder: '请输入 RTMP 推流地址' })
          .width(Common.FULL_PERCENT)
          .fontSize(14)
          .onChange((value: string) => {
            this.urlDraft = value;
          });

        Row({ space: 10 }) {
          Checkbox()
            .select(this.roiEnabledDraft)
            .selectedColor($r('app.color.button_clickable'))
            .onChange((isChecked: boolean) => {
              this.roiEnabledDraft = isChecked;
            });

          Text('启用 ROI')
            .fontSize(15)
            .fontColor($r('app.color.status_font_color'));
        }
        .width(Common.FULL_PERCENT);

        Column({ space: 8 }) {
          Row() {
            Text('Offset')
              .fontSize(14)
              .fontColor($r('app.color.status_font_color'));
            Text(`${this.roiOffsetDraft.toFixed(0)}`)
              .fontSize(14)
              .fontColor($r('app.color.status_font_color'));
          }
          .width(Common.FULL_PERCENT)
          .justifyContent(FlexAlign.SpaceBetween)

          Slider({
            value: this.roiOffsetDraft,
            min: -51,
            max: 51,
            step: 1
          })
            .blockColor($r('app.color.button_clickable'))
            .trackColor('#CCD5DD')
            .selectedColor($r('app.color.button_clickable'))
            .enabled(this.roiEnabledDraft)
            .onChange((value: number, _mode: SliderChangeMode) => {
              this.roiOffsetDraft = value;
            });
        }
        .width(Common.FULL_PERCENT)
        .opacity(this.roiEnabledDraft ? 1 : 0.45);

        Column({ space: 10 }) {
          Text('推流格式')
            .fontSize(14)
            .fontColor($r('app.color.status_font_color'));

          Row({ space: 18 }) {
            Row({ space: 6 }) {
              Radio({ value: 'H264', group: 'demo_video_codec' })
                .checked(this.videoCodecDraft === VideoCodecType.HJCodecH264)
                .height(16)
                .aspectRatio(1)
                .onChange((isChecked: boolean) => {
                  if (isChecked) {
                    this.videoCodecDraft = VideoCodecType.HJCodecH264;
                  }
                });
              Text('H.264')
                .fontSize(14)
                .fontColor($r('app.color.status_font_color'));
            }

            Row({ space: 6 }) {
              Radio({ value: 'H265', group: 'demo_video_codec' })
                .checked(this.videoCodecDraft === VideoCodecType.HJVCodecH265)
                .height(16)
                .aspectRatio(1)
                .onChange((isChecked: boolean) => {
                  if (isChecked) {
                    this.videoCodecDraft = VideoCodecType.HJVCodecH265;
                  }
                });
              Text('H.265')
                .fontSize(14)
                .fontColor($r('app.color.status_font_color'));
            }
          }
          .width(Common.FULL_PERCENT);
        }
        .width(Common.FULL_PERCENT)

        Row({ space: 12 }) {
          Button('取消')
            .layoutWeight(1)
            .height(40)
            .backgroundColor('#E8EEF2')
            .fontColor($r('app.color.status_font_color'))
            .onClick(() => {
              this.priCloseSettingsDialog();
            });

          Button(this.pendingEncodingStart ? '保存并开播' : '保存')
            .layoutWeight(1)
            .height(40)
            .backgroundColor($r('app.color.button_clickable'))
            .fontColor(Color.White)
            .onClick(() => {
              this.priSaveSettings();
            });
        }
        .width(Common.FULL_PERCENT)
      }
      .width('88%')
      .padding(20)
      .margin({ bottom: 32 })
      .backgroundColor(Color.White)
      .borderRadius(20);
    }
    .justifyContent(FlexAlign.End)
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
    .backgroundColor('#66000000');
  }

  @Builder
  private priFaceuDialog() {
    Column() {
      Blank()
        .width(Common.FULL_PERCENT)
        .layoutWeight(1)
        .onClick(() => {
          this.priCloseFaceuDialog();
        });

      Column({ space: 16 }) {
        Text('脸萌路径设置')
          .fontSize(20)
          .fontWeight(Common.FONT_WEIGHT_500)
          .fontColor($r('app.color.status_font_color'))

        TextInput({ text: this.faceuUrlDraft, placeholder: '请输入脸萌资源路径' })
          .width(Common.FULL_PERCENT)
          .fontSize(14)
          .onChange((value: string) => {
            this.faceuUrlDraft = value;
          });

        Row({ space: 12 }) {
          Button('取消')
            .layoutWeight(1)
            .height(40)
            .backgroundColor('#E8EEF2')
            .fontColor($r('app.color.status_font_color'))
            .onClick(() => {
              this.priCloseFaceuDialog();
            });

          Button('开启')
            .layoutWeight(1)
            .height(40)
            .backgroundColor($r('app.color.button_clickable'))
            .fontColor(Color.White)
            .onClick(() => {
              this.priOpenFaceuFlow();
            });
        }
        .width(Common.FULL_PERCENT)
      }
      .width('88%')
      .padding(20)
      .margin({ bottom: 32 })
      .backgroundColor(Color.White)
      .borderRadius(20);
    }
    .justifyContent(FlexAlign.End)
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
    .backgroundColor('#66000000');
  }

  @Builder
  private priPngGiftDialog() {
    Column() {
      Blank()
        .width(Common.FULL_PERCENT)
        .layoutWeight(1)
        .onClick(() => {
          this.priClosePngGiftDialog();
        });

      Column({ space: 16 }) {
        Text('礼物 PNG 路径设置')
          .fontSize(20)
          .fontWeight(Common.FONT_WEIGHT_500)
          .fontColor($r('app.color.status_font_color'))

        TextInput({ text: this.pngGiftUrlDraft, placeholder: '请输入礼物 PNG 路径' })
          .width(Common.FULL_PERCENT)
          .fontSize(14)
          .onChange((value: string) => {
            this.pngGiftUrlDraft = value;
          });

        Row({ space: 12 }) {
          Button('取消')
            .layoutWeight(1)
            .height(40)
            .backgroundColor('#E8EEF2')
            .fontColor($r('app.color.status_font_color'))
            .onClick(() => {
              this.priClosePngGiftDialog();
            });

          Button('打开')
            .layoutWeight(1)
            .height(40)
            .backgroundColor($r('app.color.button_clickable'))
            .fontColor(Color.White)
            .onClick(() => {
              this.priOpenPngGiftFlow();
            });
        }
        .width(Common.FULL_PERCENT)
      }
      .width('88%')
      .padding(20)
      .margin({ bottom: 32 })
      .backgroundColor(Color.White)
      .borderRadius(20);
    }
    .justifyContent(FlexAlign.End)
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
    .backgroundColor('#66000000');
  }

  build() {
    Stack() {
      Column() {
        Stack() {
          if (this.isRenderSurfaceMounted) {
            XComponent({
              id: `${Common.OPENGL_XCOMPONENT_ID}_${this.renderSurfaceVersion}`,
              type: XComponentType.SURFACE,
              controller: this.controller
            })
              .width(this.xComponentWidth)
              .height(this.xComponentHeight)
              .backgroundColor('#101820')
              .border({
                width: 2,
                color: '#34C759'
              })
              .borderRadius($r('app.float.xcomponent_border_radius'));
          }

          if (!this.isPreviewOpen || !this.isRenderSurfaceMounted) {
            Column()
              .width(this.xComponentWidth)
              .height(this.xComponentHeight)
              .backgroundColor(Color.Black)
              .border({
                width: 2,
                color: '#34C759'
              })
              .borderRadius($r('app.float.xcomponent_border_radius'));
          }
        }
        .width(Common.FULL_PERCENT)
        .layoutWeight(1);

        Row() {
          Text(this.currentStatus)
            .fontSize($r('app.float.status_font_size'))
            .fontColor(Color.White)
            .maxLines(1);
        }
        .width(Common.FULL_PERCENT)
        .padding({ left: 16, right: 16, top: 10, bottom: 110 });
      }
      .width(Common.FULL_PERCENT)
      .height(Common.FULL_PERCENT)
      .padding({ left: 12, right: 12, top: 12 });

      Column() {
        Scroll() {
          Row({ space: 10 }) {
            this.priOrientationButton();

            if (this.isPreviewOpen) {
              this.priActionButton('关闭预览', DemoActionType.Preview);
            } else {
              this.priActionButton('开启预览', DemoActionType.Preview);
            }

            if (this.isEncodingOpen) {
              this.priActionButton('关闭推流', DemoActionType.Encoding);
            } else {
              this.priActionButton('开启推流', DemoActionType.Encoding);
            }

            if (this.isRecorderOpen) {
              this.priActionButton('关闭录制', DemoActionType.Record);
            } else {
              this.priActionButton('打开录制', DemoActionType.Record);
            }

            this.priActionButton(this.isFrontCamera ? '后置' : '前置', DemoActionType.Camera);

            if (this.isFaceDetectOpen) {
              this.priActionButton('关闭检测', DemoActionType.FaceDetect);
            } else {
              this.priActionButton('开启检测', DemoActionType.FaceDetect);
            }

            if (this.isFaceProtectedOpen) {
              this.priActionButton('关闭人脸保护', DemoActionType.FaceProtected);
            } else {
              this.priActionButton('开启人脸保护', DemoActionType.FaceProtected);
            }

            if (this.isFaceuOpen) {
              this.priActionButton('关闭脸萌', DemoActionType.Faceu);
            } else {
              this.priActionButton('开启脸萌', DemoActionType.Faceu);
            }

            this.priActionButton('礼物PNG', DemoActionType.GiftPng);
            this.priWindowSizeButton();
            this.priDisplayModeButton();
            this.priRandomTestSelect();
          }
          .padding({ left: 4, right: 4 });
        }
        .scrollable(ScrollDirection.Horizontal)
        .scrollBar(BarState.Off)
        .width(Common.FULL_PERCENT)
        .padding(14)
        .backgroundColor('#AA182431')
        .borderRadius(24);
      }
      .width(Common.FULL_PERCENT)
      .padding({ left: 12, right: 12, bottom: 16 })
      .alignItems(HorizontalAlign.Center)
      .justifyContent(FlexAlign.End)
      .height(Common.FULL_PERCENT);

      if (this.showSettingsDialog) {
        this.priSettingsDialog();
      }

      if (this.showFaceuDialog) {
        this.priFaceuDialog();
      }

      if (this.showPngGiftDialog) {
        this.priPngGiftDialog();
      }
    }
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
    .backgroundColor('#0F1720');
  }
}
