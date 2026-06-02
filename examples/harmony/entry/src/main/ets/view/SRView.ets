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
import { CommonConstant as Common } from '../common/CommonConstant';
import SRNative from 'libHJInferenceRender.so';

const SR_XCOMPONENT_ID: string = 'PLAYER_XCOMPONENT_ID';
const SR_LIBRARY_NAME: string = 'HJInferenceRender';

interface SRInfo {
  ret?: number;
  faceCount?: number;
  faceDetectMs?: number;
  srMs?: number;
  faceX?: number;
  faceY?: number;
  faceW?: number;
  faceH?: number;
  scaleW?: number;
  scaleH?: number;
}

interface SRNativeApi {
  srInit(modelDir: string, seqDir: string, fps: number, loop: boolean, fullSR: boolean, faceScale: boolean, faceScaleWidth: number): number;
  srStart(): number;
  srStop(): number;
  srGetInfo(): SRInfo | null;
}

const SRApi: SRNativeApi = SRNative as SRNativeApi;

@Component
export struct SRView {
  @State currentStatus: string = 'SR idle';
  @State xComponentWidth: Resource = $r('app.float.xcomponent_width');
  @State xComponentHeight: Resource = $r('app.float.xcomponent_height');

  @State modelDir: string = '/data/storage/el2/base/haps/entry/files/resource/model';
  @State seqDir: string = '/data/storage/el2/base/haps/entry/files/resource/imgseq/360x640';
  @State fps: number = 15;
  @State loop: boolean = true;
  @State fullSR: boolean = false;
  @State faceScale: boolean = true;
  @State faceScaleWidth: number = 90;

  @State srRet: number = 0;
  @State faceCount: number = 0;
  @State fdMs: number = 0;
  @State srMs: number = 0;
  @State faceRectInfo: string = '-';
  @State scaleInfo: string = '-';

  @State isRunning: boolean = false;

  private xComponentContext: XComponentContext | undefined = undefined;
  private m_idx: number = 0;
  private infoTimer: number = -1;

  aboutToDisappear(): void {
    this.stopInfoPoll();
    if (this.isRunning) {
      SRApi.srStop();
      this.isRunning = false;
    }
  }

  private normalizeFaceScale(v: number): number {
    let n = Math.round(v);
    if (n < 70) {
      n = 70;
    }
    if (n > 100) {
      n = 100;
    }
    if ((n % 2) !== 0) {
      n = (n === 100) ? 98 : (n + 1);
    }
    return n;
  }

  private doInit() {
    const safeFps = Math.max(1, Math.round(this.fps));
    const safeScale = this.normalizeFaceScale(this.faceScaleWidth);
    this.fps = safeFps;
    this.faceScaleWidth = safeScale;
    const ret: number = SRApi.srInit(this.modelDir, this.seqDir, safeFps, this.loop, this.fullSR, this.faceScale, safeScale);
    this.srRet = ret;
    this.currentStatus = `srInit ret=${ret}`;
  }

  private doStart() {
    const ret: number = SRApi.srStart();
    this.srRet = ret;
    this.currentStatus = `srStart ret=${ret}`;
    if (ret >= 0) {
      this.isRunning = true;
      this.startInfoPoll();
    }
  }

  private doStop() {
    const ret: number = SRApi.srStop();
    this.srRet = ret;
    this.currentStatus = `srStop ret=${ret}`;
    this.isRunning = false;
    this.stopInfoPoll();
  }

  private startInfoPoll() {
    this.stopInfoPoll();
    this.infoTimer = setInterval(() => {
      const info: SRInfo | null = SRApi.srGetInfo();
      if (!info) {
        return;
      }
      this.srRet = Number(info.ret ?? this.srRet);
      this.faceCount = Number(info.faceCount ?? 0);
      this.fdMs = Number(info.faceDetectMs ?? 0);
      this.srMs = Number(info.srMs ?? 0);
      const fx = Number(info.faceX ?? 0);
      const fy = Number(info.faceY ?? 0);
      const fw = Number(info.faceW ?? 0);
      const fh = Number(info.faceH ?? 0);
      this.faceRectInfo = `${fx},${fy},${fw},${fh}`;
      const sw = Number(info.scaleW ?? 0);
      const sh = Number(info.scaleH ?? 0);
      this.scaleInfo = `${sw}x${sh}`;
      this.currentStatus = `ret=${this.srRet} face=${this.faceCount} fd=${this.fdMs}ms sr=${this.srMs}ms`;
    }, 300) as number;
  }

  private stopInfoPoll() {
    if (this.infoTimer >= 0) {
      clearInterval(this.infoTimer);
      this.infoTimer = -1;
    }
  }

  private toggleWindowSize() {
    if ((this.m_idx % 3) == 0) {
      this.xComponentWidth = $r('app.float.xcomponent_width2');
      this.xComponentHeight = $r('app.float.xcomponent_height2');
    } else if ((this.m_idx % 3) == 1) {
      this.xComponentWidth = $r('app.float.xcomponent_width3');
      this.xComponentHeight = $r('app.float.xcomponent_height3');
    } else {
      this.xComponentWidth = $r('app.float.xcomponent_width');
      this.xComponentHeight = $r('app.float.xcomponent_height');
    }
    this.m_idx++;
  }

  build() {
    Column() {
      Column() {
        XComponent({
          id: SR_XCOMPONENT_ID,
          type: XComponentType.SURFACE,
          libraryname: SR_LIBRARY_NAME
        })
          .width(this.xComponentWidth)
          .height(this.xComponentHeight)
          .onLoad((xComponentContext) => {
            this.xComponentContext = xComponentContext as XComponentContext;
          })
          .borderRadius($r('app.float.xcomponent_border_radius'))
          .backgroundColor(Color.White)

        Text(this.currentStatus)
          .fontSize($r('app.float.status_font_size'))
          .fontColor($r('app.color.status_font_color'))
          .fontWeight(Common.FONT_WEIGHT_500)
          .opacity(Common.TEXT_OPACITY)
          .margin({ top: $r('app.float.status_margin_top') })
      }
      .height('62%')

      Scroll() {
        Column({ space: 10 }) {
          Row() {
            Text('modelDir').fontSize(15).width('25%')
            TextInput({ text: this.modelDir })
              .fontSize(14)
              .width('72%')
              .onChange((value: string) => {
                this.modelDir = value;
              })
          }
          .width('100%')
          .justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Text('seqDir').fontSize(15).width('25%')
            TextInput({ text: this.seqDir })
              .fontSize(14)
              .width('72%')
              .onChange((value: string) => {
                this.seqDir = value;
              })
          }
          .width('100%')
          .justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Text(`fps ${this.fps}`).fontSize(15)
            Slider({ value: this.fps, min: 1, max: 60, step: 1 })
              .width('56%')
              .onChange((value: number) => {
                this.fps = Math.round(value);
              })
            Toggle({ type: ToggleType.Switch, isOn: this.loop })
              .onChange((value) => {
                this.loop = value;
              })
            Text('loop').fontSize(13)
          }
          .width('100%')
          .justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Text(`faceScale ${this.faceScaleWidth}`).fontSize(15)
            Slider({ value: this.faceScaleWidth, min: 70, max: 100, step: 1 })
              .width('40%')
              .onChange((value: number) => {
                this.faceScaleWidth = this.normalizeFaceScale(value);
              })
            Toggle({ type: ToggleType.Switch, isOn: this.faceScale })
              .onChange((value) => {
                this.faceScale = value;
              })
            Text('face').fontSize(13)
            Toggle({ type: ToggleType.Switch, isOn: this.fullSR })
              .onChange((value) => {
                this.fullSR = value;
              })
            Text('full').fontSize(13)
          }
          .width('100%')
          .justifyContent(FlexAlign.SpaceBetween)

          Row({ space: 8 }) {
            Button('SR Init')
              .width('24%')
              .onClick(() => {
                this.doInit();
              })
            Button('SR Start')
              .width('24%')
              .onClick(() => {
                this.doStart();
              })
            Button('SR Stop')
              .width('24%')
              .onClick(() => {
                this.doStop();
              })
            Button('Window')
              .width('24%')
              .onClick(() => {
                this.toggleWindowSize();
              })
          }
          .width('100%')

          Text(`ret=${this.srRet} running=${this.isRunning} faceCount=${this.faceCount}`)
            .fontSize(14)
          Text(`fd=${this.fdMs}ms sr=${this.srMs}ms`)
            .fontSize(14)
          Text(`faceRect=${this.faceRectInfo} scale=${this.scaleInfo}`)
            .fontSize(14)
        }
        .width('100%')
      }
      .layoutWeight(1)
      .width('100%')
    }
    .padding({ left: 12, right: 12, top: 8, bottom: 8 })
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
  }
}
