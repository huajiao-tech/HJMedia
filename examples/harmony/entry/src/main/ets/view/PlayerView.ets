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
import HJPlayer from 'libHJPlayer.so';
import { abilityAccessCtrl } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import window from '@ohos.window';
import common from '@ohos.app.ability.common';
import image from '@ohos.multimedia.image';
import { faceDetector } from '@kit.CoreVisionKit';
//import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from 'hjmediautils'
import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from '../../../../../hjpusher/src/main/ets/HJFaceDetectMgr'
@Component
export struct PlayerView {
  @State currentStatus: ResourceStr = '';
  @State currentIndex: number = 0;
  @State isClick: boolean = true;

  @State xComponentWidth: Resource = $r('app.float.xcomponent_width');
  @State xComponentHeight: Resource = $r('app.float.xcomponent_height');

  private m_idx: number = 0;



  private xComponentContext: XComponentContext | undefined = undefined;

  @State isScreenOn: boolean = false
  private context: common.BaseContext = getContext(this) as common.BaseContext
  aboutToAppear(): void {
    try {
      this.keepScreenOn();
      let atManager = abilityAccessCtrl.createAtManager();
      atManager.requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA', 'ohos.permission.MICROPHONE', 'ohos.permission.INTERNET'])
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
    await windowClass.setWindowKeepScreenOn(true)
    this.isScreenOn = await windowClass.getWindowProperties().isKeepScreenOn
  }

  private m_faceDetectRet : number = HJFaceDetectMgrRet.ok;
  private m_bFaceOpen:boolean = false;
  private m_faceDetectMgr : HJFaceDetectMgr = new HJFaceDetectMgr();

  build()
  {
      Column() {
        Column() {
          XComponent({
            id: Common.PLAYER_XCOMPONENT_ID,
            type: XComponentType.SURFACE,
            libraryname: Common.LIBRARY_NAME_PLAYER
          })
            .width(this.xComponentWidth)
            .height(this.xComponentHeight)
            //.aspectRatio(0.56)
            .onLoad((xComponentContext) => {
              this.xComponentContext = xComponentContext as XComponentContext;
              this.currentStatus = ' ';
            })
            .borderRadius($r('app.float.xcomponent_border_radius'))
            .backgroundColor(Color.White)

          Text(this.currentStatus)
            .fontSize($r('app.float.status_font_size'))
            .fontColor($r('app.color.status_font_color'))
            .fontWeight(Common.FONT_WEIGHT_500)
            .opacity(Common.TEXT_OPACITY)
            .margin({
              top: $r('app.float.status_margin_top')
            })
        }
        .height('80%')

        Column()
        {
          Row() {
            Button($r('app.string.testPlayerOpen'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                HJPlayer.testOpen();
              })
              .width(Common.BUTTON_WIDTH_EX)
              .height($r('app.float.button_height'))
              .margin({
                bottom:$r('app.float.button_margin_bottom')
              })
              .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
              .stateEffect(this.isClick)

            Button($r('app.string.testEffectGray'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                HJPlayer.testEffectGray();
              })
              .width(Common.BUTTON_WIDTH_EX)
              .height($r('app.float.button_height'))
              .margin({
                bottom:$r('app.float.button_margin_bottom')
              })
              .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
              .stateEffect(this.isClick)

            Button($r('app.string.testEffectBlur'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                HJPlayer.testEffectBlur();
              })
              .width(Common.BUTTON_WIDTH_EX)
              .height($r('app.float.button_height'))
              .margin({
                bottom:$r('app.float.button_margin_bottom')
              })
              .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
              .stateEffect(this.isClick)


            Button($r('app.string.testPlayerClose'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                HJPlayer.testClose();
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
          .layoutWeight(1)

          //////////////////////////////////////////////////////////

          Row() {
            Button($r('app.string.tryPlayerOpen'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                HJPlayer.tryOpen();
              })
              .width(Common.BUTTON_WIDTH_EX)
              .height($r('app.float.button_height'))
              .margin({
                bottom:$r('app.float.button_margin_bottom')
              })
              .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
              .stateEffect(this.isClick)

            Button($r('app.string.tryPlayerFaceu'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(async () => {
                if (!this.m_bFaceOpen)
                {
                  this.m_faceDetectMgr.registerFaceInfoCb((width:number, height:number, faceInfo:string)=>
                  {
                    HJPlayer.trySetFacePoints(width, height, faceInfo);
                  });
                  this.m_faceDetectMgr.registerNativeSourceOpenCb(() =>
                  {
                    HJPlayer.tryOpenImgReceiver();
                    return 0;
                  });
                  this.m_faceDetectMgr.registerNativeSourceAcquireCb((): HJNativeSourceData =>
                  {
                    return HJPlayer.tryGetMediaData();
                  });
                  this.m_faceDetectMgr.registerNativeSourceCloseCb(()=>
                  {
                    HJPlayer.tryCloseImageReceiver();
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

            Button($r('app.string.tryWindowChange'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() =>
              {
                if ((this.m_idx % 3) == 0)
                {
                  this.xComponentWidth  = $r('app.float.xcomponent_width2');
                  this.xComponentHeight  = $r('app.float.xcomponent_height2');
                }
                else if ((this.m_idx % 3) == 1)
                {
                  this.xComponentWidth  = $r('app.float.xcomponent_width3');
                  this.xComponentHeight  = $r('app.float.xcomponent_height3');
                }
                else if ((this.m_idx % 3) == 2)
                {
                  this.xComponentWidth  = $r('app.float.xcomponent_width');
                  this.xComponentHeight  = $r('app.float.xcomponent_height');
                }
                this.m_idx++;
              })
              .width(Common.BUTTON_WIDTH_EX)
              .height($r('app.float.button_height'))
              .margin({
                bottom:$r('app.float.button_margin_bottom')
              })
              .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
              .stateEffect(this.isClick)

            Button($r('app.string.tryPlayerClose'))
              .fontSize($r('app.float.button_font_size'))
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                HJPlayer.tryClose();
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
          .layoutWeight(1)


          // //Blank().height('20vp') // 添加固定间距
          //
          // Row() {
          //   Button($r('app.string.tryPreview'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .fontWeight(Common.FONT_WEIGHT_500)
          //     .onClick(() => {
          //       if (this.xComponentContext)
          //       {
          //         let previewSurfaceId: bigint = HJPusher.tryOpenPreview();
          //         cameraService.initCamera(getContext() as Context);
          //         cameraService.bindSurfaceId(previewSurfaceId.toString());
          //         cameraService.startPreview(1);
          //       }
          //     })
          //     .width(Common.BUTTON_WIDTH)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          //
          //   Button($r('app.string.tryPush'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .fontWeight(Common.FONT_WEIGHT_500)
          //     .onClick(() => {
          //       HJPusher.tryOpenPush();
          //     })
          //     .width(Common.BUTTON_WIDTH)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          //
          //   Button($r('app.string.tryRecordOpen'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .fontWeight(Common.FONT_WEIGHT_500)
          //     .onClick(() => {
          //       HJPusher.tryOpenRecord();
          //     })
          //     .width(Common.BUTTON_WIDTH)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          //
          //   Button($r('app.string.tryRecordClose'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .fontWeight(Common.FONT_WEIGHT_500)
          //     .onClick(() => {
          //       HJPusher.tryCloseRecord();
          //     })
          //     .width(Common.BUTTON_WIDTH)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          //
          //   Button($r('app.string.tryClosePreview'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .fontWeight(Common.FONT_WEIGHT_500)
          //     .onClick(() => {
          //       cameraService.releaseCamera();
          //       HJPusher.tryClosePreview();
          //     })
          //     .width(Common.BUTTON_WIDTH)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          // }
          // .alignItems(VerticalAlign.Bottom)
          // .layoutWeight(1)
          //
          //
          // //Blank().height('20vp') // 添加固定间距
          // Row() {
          //   Button($r('app.string.tryGift'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .fontWeight(Common.FONT_WEIGHT_500)
          //     .onClick(() => {
          //       HJPusher.tryGiftOpen();
          //     })
          //     .width(Common.BUTTON_WIDTH_EX)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          //
          //   Button($r('app.string.tryDoubleScreen'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .fontWeight(Common.FONT_WEIGHT_500)
          //     .onClick(() => {
          //       HJPusher.tryDoubleScreen();
          //     })
          //     .width(Common.BUTTON_WIDTH_EX)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          //
          //   Button($r('app.string.tryGiftPusher'))
          //     .fontSize($r('app.float.button_font_size'))
          //     .onClick(() => {
          //       HJPusher.tryGiftPusher();
          //     })
          //     .width(Common.BUTTON_WIDTH_EX)
          //     .height($r('app.float.button_height'))
          //     .margin({
          //       bottom:$r('app.float.button_margin_bottom')
          //     })
          //     .backgroundColor(this.isClick ? $r('app.color.button_clickable') : $r('app.color.button_unclickable'))
          //     .stateEffect(this.isClick)
          // }
          // .alignItems(VerticalAlign.Bottom)
          // .layoutWeight(1)
          //
          // //Blank().height('20vp') // 添加固定间距
        }
        .layoutWeight(1)  // 占用剩余空间
        .width('100%')
      }
      .width(Common.FULL_PERCENT)
      .height(Common.FULL_PERCENT)
  }
}