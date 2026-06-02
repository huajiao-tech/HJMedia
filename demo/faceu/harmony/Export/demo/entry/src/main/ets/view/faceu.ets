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
import HJNativeRender from 'libnativerender.so';

interface FaceuItem {
  key: string;
  dir: string;
}

interface NativeRender {
  nFaceuOpen?: () => void;
  nFaceuClose?: () => void;
  nFaceuRemoveAll?: () => void;
  nFaceuAdd?: (key: string, path: string, useDebugPoints: boolean) => void;
  nFaceuRemove?: (key: string) => void;
  nFaceuSetImgSeqVisible?: (show: boolean) => void;
}

@Component
export struct OpenGLView {
  private xComponentContext: XComponentContext | undefined = undefined;
  @State private useDebugPoints: boolean = false;
  private readonly faceuBasePath: string = '/data/storage/el1/bundle/entry/resources/resfile/resource/faceu/';
  private readonly faceuList: Array<FaceuItem> = [
    { key: 'face_0', dir: '60031_10' },
    { key: 'face_1', dir: '90237_1' },
    { key: 'face_2', dir: 'XianWeng' }
  ];
  private readonly compactButtonHeight: string = '34vp';
  private readonly compactFontSize: number = 13;
  private readonly compactButtonWidth: string = '38%';
  private readonly tripleRowButtonWidth: string = '30%';
  private readonly nativeRender: NativeRender = HJNativeRender as NativeRender;
  @State private showImgSeq: boolean = false;

  build() {
    Stack({ alignContent: Alignment.Top }) {
      XComponent({
        id: Common.OPENGL_XCOMPONENT_ID,
        type: XComponentType.TEXTURE,
        libraryname: Common.LIBRARY_NAME
      })
        .width($r('app.float.xcomponent_width'))
        .height($r('app.float.xcomponent_height'))
        .onLoad((xComponentContext) => {
          this.xComponentContext = xComponentContext as XComponentContext;
        })
        .borderRadius($r('app.float.xcomponent_border_radius'))
        .backgroundColor(Color.White)

      Column() {
        Row() {
          Checkbox({ name: 'imgseq' })
            .select(this.showImgSeq)
            .onChange((value: boolean) => {
              this.showImgSeq = value;
              if (this.nativeRender.nFaceuSetImgSeqVisible) {
                this.nativeRender.nFaceuSetImgSeqVisible(value);
              }
            })
          Text('imgseq')
            .fontSize(this.compactFontSize)
            .fontWeight(Common.FONT_WEIGHT_500)
            .margin({ left: 6 })

          Checkbox({ name: 'debugPoints' })
            .select(this.useDebugPoints)
            .onChange((value: boolean) => {
              this.useDebugPoints = value;
            })
          Text($r('app.string.faceudebugpoints'))
            .fontSize(this.compactFontSize)
            .fontWeight(Common.FONT_WEIGHT_500)
            .margin({ left: 12 })
        }
        .width(Common.FULL_PERCENT)
        .alignItems(VerticalAlign.Center)
        .margin({ bottom: 6 })

        Row() {
          Button($r('app.string.faceuopen'))
            .fontSize(this.compactFontSize)
            .fontWeight(Common.FONT_WEIGHT_500)
            .onClick(() => {
              if (this.nativeRender.nFaceuOpen) {
                this.nativeRender.nFaceuOpen();
              }
            })
            .width(this.tripleRowButtonWidth)
            .height(this.compactButtonHeight)
          Button($r('app.string.faceuclose'))
            .fontSize(this.compactFontSize)
            .fontWeight(Common.FONT_WEIGHT_500)
            .onClick(() => {
              if (this.nativeRender.nFaceuClose) {
                this.nativeRender.nFaceuClose();
              }
            })
            .width(this.tripleRowButtonWidth)
            .height(this.compactButtonHeight)
          Button($r('app.string.faceuremoveall'))
            .fontSize(this.compactFontSize)
            .fontWeight(Common.FONT_WEIGHT_500)
            .onClick(() => {
              if (this.nativeRender.nFaceuRemoveAll) {
                this.nativeRender.nFaceuRemoveAll();
              }
            })
            .width(this.tripleRowButtonWidth)
            .height(this.compactButtonHeight)
        }
        .width(Common.FULL_PERCENT)
        .justifyContent(FlexAlign.SpaceBetween)
        .margin({ bottom: 6 })

        ForEach(this.faceuList, (item: FaceuItem) => {
          Row() {
            Button($r('app.string.faceuadd'))
              .fontSize(this.compactFontSize)
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                this.handleFaceuAdd(item);
              })
              .width(this.compactButtonWidth)
              .height(this.compactButtonHeight)
            Button($r('app.string.faceuremove'))
              .fontSize(this.compactFontSize)
              .fontWeight(Common.FONT_WEIGHT_500)
              .onClick(() => {
                this.handleFaceuRemove(item);
              })
              .width(this.compactButtonWidth)
              .height(this.compactButtonHeight)
          }
          .width(Common.FULL_PERCENT)
          .justifyContent(FlexAlign.SpaceBetween)
          .margin({ bottom: 6 })
        }, (item: FaceuItem) => item.key)
      }
      .alignItems(HorizontalAlign.Start)
      .padding({ left: 12, right: 12, top: 12, bottom: 8 })
    }
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
  }

  private handleFaceuAdd(item: FaceuItem) {
    if (this.nativeRender.nFaceuAdd) {
      const path = this.faceuBasePath + item.dir;
      this.nativeRender.nFaceuAdd(item.key, path, this.useDebugPoints);
    }
  }

  private handleFaceuRemove(item: FaceuItem) {
    if (this.nativeRender.nFaceuRemove) {
      this.nativeRender.nFaceuRemove(item.key);
    }
  }
}
