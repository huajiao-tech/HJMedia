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

import { OpenGLView } from '../view/OpenGLView';
import { CommonConstant as Common } from '../common/CommonConstant';
import { NavStackUtil } from '../util/NavStackUitl';

@Entry
@ComponentV2
struct Index {
  @Local currentIndex: number = 0;

  @Builder
  tabBuilder(index: number, name: ResourceStr) {
    Column() {
      Row() {
        Text(name)
          .fontColor(this.currentIndex === index ? $r('app.color.selected_font_color') : $r('app.color.font_color'))
          .fontSize($r('app.float.tab_bar_font_size'))
          .fontWeight(this.currentIndex === index ? Common.FONT_WEIGHT_500 : FontWeight.Normal)
          .lineHeight($r('app.float.tab_bar_line_height'))
          .margin({ top: $r('app.float.tab_bar_title_margin_top') })
      }
      .height($r('app.float.tab_bar_title_height'))
      .alignItems(VerticalAlign.Top)
      .borderWidth({
        bottom: this.currentIndex === index ? $r('app.float.tab_bar_title_border_width') : $r('app.float.zero')
      })
      .borderColor($r('app.color.tab_bar_title_border_color'))
    }
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
  }

  build() {
    Column() {
      Row() {
        Text($r('app.string.page_title'))
          .fontSize($r('app.float.page_title_font_size'))
          .fontWeight(Common.FONT_WEIGHT_500)
          .margin({
            left: $r('app.float.page_title_margin_left')
          })
        Button("Demo页面")
          .onClick(() => {
            this.getUIContext().getRouter().pushUrl({url: "pages/IndexV2"})
          })
      }
      .margin({
        top: $r('app.float.page_title_margin_top')
      })
      .width(Common.FULL_PERCENT)
      .height($r('app.float.page_title_height'))
      .justifyContent(FlexAlign.Start)

      Tabs({ barPosition: BarPosition.Start, index: this.currentIndex }) {
        TabContent() {
          OpenGLView()
        }
        .tabBar(this.tabBuilder(0, $r('app.string.opengl_draw')))
      }
      .vertical(false)
      .barMode(BarMode.Fixed)
      .barWidth(Common.FULL_PERCENT)
      .barHeight($r('app.float.bar_height'))
      .onChange((index: number) => {
        this.currentIndex = index;
      })
      .width(Common.FULL_PERCENT)
      .layoutWeight(1)
      .margin({
        top: $r('app.float.tabs_margin_top')
      })
    }
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
    .backgroundColor('#F0F3F5')
  }
}