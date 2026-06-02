/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import XComponentContext from '../interface/XComponentContext';
import { CommonConstant as Common }  from '../common/CommonConstant';
import { cameraService } from '../camera/service/CameraService';
import HJPusher from 'libHJPusher.so';
import { abilityAccessCtrl } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { window } from '@kit.ArkUI';
import common from '@ohos.app.ability.common';
//import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from 'hjmediautils'
import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from '../../../../../hjmediautils/src/main/ets/HJFaceDetectMgr'
import { WindowUtils } from '../util/WindowUtils';
import { HJRteGraphConfigConstructor, HJRteGraphConstructorType } from 'hjpusher/src/main/ets/native/HJRteGraphConfigConstructor';
import { HJRteGraphConfig } from 'hjpusher/src/main/ets/native/HJRteGraphSetupInfo';

@Component
export struct PusherView {
  @State currentStatus: ResourceStr = '';
  @State currentIndex: number = 0;
  @State isClick: boolean = true;
  @State sliderValue: number = 51;
  @State matchValue: number = 1.0;
  private xComponentContext: XComponentContext | undefined = undefined;

  @State isScreenOn: boolean = false
  @State isLandscape: boolean = false
  @State orientationMessage: string = ''
  private context: common.BaseContext = getContext(this) as common.BaseContext
  private windowClass?: window.Window
  private onWindowSizeChange = (size: window.Size) => {
    this.updateOrientationBySize(size.width, size.height);
  }
  aboutToAppear(): void {
    try {
      this.keepScreenOn();
      let atManager = abilityAccessCtrl.createAtManager();
      atManager.requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA', 'ohos.permission.MICROPHONE'])
        .then((data) => {
          // Logger.info('Entry', 'requestPre() data: ' + JSON.stringify(data));
        }).catch((err: BusinessError) => {
        // Logger.info('Entry', 'requestPre() data: ' + JSON.stringify(err));
      })
    } catch (err) {
      // Logger.error('Entry', 'requestPre() data: ' + JSON.stringify(err));
    }
  }
  async keepScreenOn() {
    let windowClass = await window.getLastWindow(this.context)
    this.windowClass = windowClass
    await windowClass.setWindowKeepScreenOn(true)
    this.isScreenOn = await windowClass.getWindowProperties().isKeepScreenOn
    let rect = windowClass.getWindowProperties().windowRect
    this.updateOrientationBySize(rect.width, rect.height)
    windowClass.on('windowSizeChange', this.onWindowSizeChange)
  }

  aboutToDisappear(): void {
    this.windowClass?.off('windowSizeChange', this.onWindowSizeChange)
  }

  private updateOrientationBySize(width: number, height: number) {
    this.isLandscape = width > height
    this.orientationMessage = this.isLandscape ? '当前横屏' : '当前竖屏'
    let initPreviewRotation = this.updateCameraPreviewRotation();
    console.log('HVSwitchStat initPreviewRotation ret', initPreviewRotation);
    if (initPreviewRotation !== undefined) {
      HJPusher.trySetPreviewRotation(initPreviewRotation);
    }
    console.log('HVSwitchStat updateOrientationBySize', this.isLandscape, width, height)
  }

  private updateCameraPreviewRotation(): number | undefined {
    return cameraService.updatePreviewRotation();
  }

  private applySRMatch(): void {
    HJPusher.tryNodeSetParam(
      HJRteGraphConfig.HJNodeClass_FilterSR,
      HJRteGraphConfig.HJNodeClass_FilterSR,
      `{"match":${this.matchValue.toFixed(2)}}`
    );
  }

  private toggleOrientation() {
    let target = this.isLandscape ? window.Orientation.PORTRAIT : window.Orientation.LANDSCAPE
    WindowUtils.setOrientation(target)
  }

  private m_faceDetectRet : number = HJFaceDetectMgrRet.ok;
  private m_bFaceOpen:boolean = false;
  private m_faceDetectMgr : HJFaceDetectMgr = new HJFaceDetectMgr();

  build()
  {
    Stack() {
      Column() {
        Column() {
          XComponent({
            id: Common.OPENGL_XCOMPONENT_ID,
            type: XComponentType.SURFACE,
            libraryname: Common.LIBRARY_NAME
          })
            .width('100%')
            .layoutWeight(1)
            .aspectRatio(this.isLandscape ? 0.6 : 0.65)
            .onLoad((xComponentContext) => {
              this.xComponentContext = xComponentContext as XComponentContext;
              this.currentStatus = ' ';
            })
            .onAreaChange((oldValue, newValue) => {
              console.log(`HVSwitchStat XComponent area change: old=${JSON.stringify(oldValue)} new=${JSON.stringify(newValue)}`);
            })
            .borderRadius($r('app.float.xcomponent_border_radius'))
            .backgroundColor(Color.White)

          Text(this.currentStatus)
            .fontSize($r('app.float.status_font_size'))
            .fontColor($r('app.color.status_font_color'))
            .fontWeight(Common.FONT_WEIGHT_500)
            .opacity(Common.TEXT_OPACITY)
            .margin({
              top: 4
            })
        }
        .layoutWeight(this.isLandscape ? 3 : 3)
        .width('100%')

        Scroll() {
          Column() {
            Row() {
              Text(this.orientationMessage)
                .fontSize($r('app.float.status_font_size'))
                .fontColor($r('app.color.status_font_color'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .opacity(Common.TEXT_OPACITY)
            }
            .width('100%')
            .justifyContent(FlexAlign.Start)
            .padding({ left: 12, right: 12, top: 0, bottom: 4 })

            //Blank().height('20vp') // 添加固定间距

            Row() {
              Button($r('app.string.tryPreview'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(async () => {
                  if (this.xComponentContext)
                  {
                    const graphconfig: string = HJRteGraphConfigConstructor.constructGraph({
                      type: HJRteGraphConstructorType.HJRteGraphConstructorType_PlaceHolder,
                      useCustomFilter: false,
                      isMirror: false,
                      isEnableEnhance: true,
                      isEnableEnhanceMatch: true,
                      isUI0Clip: true
                    });
                    let previewSurfaceId: bigint = HJPusher.tryOpenPreview(graphconfig);
                    cameraService.initCamera(getContext() as Context);
                    cameraService.bindSurfaceId(previewSurfaceId.toString());
                    await cameraService.startPreview(1);  // 0 back,  1 front
                    let initPreviewRotation = cameraService.getLastPreviewRotation();
                    if (initPreviewRotation !== undefined) {
                      HJPusher.trySetPreviewRotation(initPreviewRotation);
                    }
                    this.applySRMatch();
                  }
                })
                .width(Common.BUTTON_WIDTH)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.tryPush'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(() => {
                  HJPusher.tryOpenPush();
                  HJPusher.trySetROIOffset(this.sliderValue);
                })
                .width(Common.BUTTON_WIDTH)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.tryRecordOpen'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(() => {
                  HJPusher.tryOpenRecord();
                })
                .width(Common.BUTTON_WIDTH)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.tryRecordClose'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(() => {
                  HJPusher.tryCloseRecord();
                })
                .width(Common.BUTTON_WIDTH)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.tryClosePreview'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(() => {
                  cameraService.releaseCamera();
                  HJPusher.tryClosePreview();
                })
                .width(Common.BUTTON_WIDTH)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)
            }
            .alignItems(VerticalAlign.Bottom)


            //Blank().height('20vp') // 添加固定间距

            Row() {
              Button($r('app.string.tryFaceu'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(async () => {
                  if (!this.m_bFaceOpen)
                  {
                      this.m_faceDetectMgr.registerFaceInfoCb((width:number, height:number, faceInfo:string)=>
                      {
                          console.log("faceinfo: " + faceInfo);
                          HJPusher.trySetFacePoints(HJRteGraphConfig.HJNodeClass_SourceBridge, width, height, faceInfo);
                      });
                      this.m_faceDetectMgr.registerNativeSourceOpenCb(() =>
                      {
                          HJPusher.tryOpenImgReceiver();
                          return 0;
                      });
                      this.m_faceDetectMgr.registerNativeSourceAcquireCb((): HJNativeSourceData =>
                      {
                          return HJPusher.tryGetMediaData();
                      });
                      this.m_faceDetectMgr.registerNativeSourceCloseCb(()=>
                      {
                          HJPusher.tryCloseImageReceiver();
                      });
                      this.m_faceDetectRet = await this.m_faceDetectMgr.openFaceDetect();
                      if (this.m_faceDetectRet != HJFaceDetectMgrRet.ok)
                      {
                          console.log("openFaceDetect error");
                      }
                  }
                  else
                  {
                      await this.m_faceDetectMgr.closeFaceDetect();
                  }
                  this.m_bFaceOpen = !this.m_bFaceOpen;
                })
                .width(Common.BUTTON_WIDTH_EX)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.tryGift'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(() => {
                  HJPusher.tryGiftOpen();
                })
                .width(Common.BUTTON_WIDTH_EX)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.tryDoubleScreen'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(() => {
                  HJPusher.tryDoubleScreen();
                })
                .width(Common.BUTTON_WIDTH_EX)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.tryGiftPusher'))
                .fontSize($r('app.float.button_font_size'))
                .onClick(() => {
                  HJPusher.tryGiftPusher();
                })
                .width(Common.BUTTON_WIDTH_EX)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)
            }
            .alignItems(VerticalAlign.Bottom)

            Row() {
              Text(`${this.sliderValue.toFixed(0)}`)
                .width(40)
              Slider({
                value: this.sliderValue,
                min: -51,
                max: 51,
                step: 1
              })
              .onChange((value: number, mode: SliderChangeMode) => {
                this.sliderValue = value;
                HJPusher.trySetROIOffset(this.sliderValue);
              })
              .width('80%')
            }
            .justifyContent(FlexAlign.Center)
            .width('100%')
            .margin({ top: 10 })

            Row() {
              Text(`${this.matchValue.toFixed(2)}`)
                .width(40)
              Slider({
                value: this.matchValue,
                min: 0,
                max: 1,
                step: 0.01
              })
              .onChange((value: number, mode: SliderChangeMode) => {
                this.matchValue = value;
                this.applySRMatch();
              })
              .width('80%')
            }
            .justifyContent(FlexAlign.Center)
            .width('100%')
            .margin({ top: 10 })

            Row() {
              Button($r('app.string.testOpen'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(async () => {
                  if (this.xComponentContext)
                  {
                    let previewSurfaceId: bigint = HJPusher.testOpen();
                    cameraService.initCamera(getContext() as Context);
                    cameraService.bindSurfaceId(previewSurfaceId.toString());
                    await cameraService.startPreview(1);          // CAMERA_POSITION_BACK = 1, CAMERA_POSITION_FRONT = 2,
                    let initPreviewRotation = cameraService.getLastPreviewRotation();
                    if (initPreviewRotation !== undefined) {
                      HJPusher.trySetPreviewRotation(initPreviewRotation);
                    }
                  }
                })
                .width(Common.BUTTON_WIDTH)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)

              Button($r('app.string.testClose'))
                .fontSize($r('app.float.button_font_size'))
                .fontWeight(Common.FONT_WEIGHT_500)
                .onClick(() => {

                  cameraService.releaseCamera();
                  HJPusher.testClose();
                })
                .width(Common.BUTTON_WIDTH)
                .height($r('app.float.button_height'))
                .margin({
                  bottom:$r('app.float.button_margin_bottom')
                })
                .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
                .stateEffect(this.isClick)
            }
            .alignItems(VerticalAlign.Bottom)

            //Blank().height('20vp') // 添加固定间距
          }
          .width('100%')
          .padding({ left: 8, right: 8, bottom: 12 })
        }
        .margin({ top: this.isLandscape ? -8 : 0 })
        .layoutWeight(this.isLandscape ? 1.5 : 1)  // 占用剩余空间
        .width('100%')
      }
      .width(Common.FULL_PERCENT)
      .height(Common.FULL_PERCENT)

      Button('切换横竖屏')
        .fontSize(12)
        .fontWeight(Common.FONT_WEIGHT_500)
        .onClick(() => {
          this.toggleOrientation()
        })
        .height(24)
        .padding({ left: 8, right: 8 })
        .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
        .stateEffect(this.isClick)
        .position({ x: 8, y: 6 })
        .zIndex(1)
    }
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
  }
}


