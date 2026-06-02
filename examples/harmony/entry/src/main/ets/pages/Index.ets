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

import { HJPlayerDemo } from '../view/HJPlayerDemo';
import { HJPusherDemo } from '../view/HJPusherDemo';
import { PusherView } from '../view/PusherView';
import { PlayerView} from '../view/PlayerView';
import { SRView} from '../view/SRView';
import { CommonConstant as Common } from '../common/CommonConstant';

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
      Tabs({ barPosition: BarPosition.Start, index: this.currentIndex }) {

        TabContent() {
          HJPusherDemo()
        }
        .tabBar(this.tabBuilder(0, $r('app.string.pusherDemo')))

        TabContent() {
          HJPlayerDemo()
        }
        .tabBar(this.tabBuilder(1, $r('app.string.playerDemo')))

        TabContent() {
          PlayerView()
        }
        .tabBar(this.tabBuilder(2, $r('app.string.player')))

        TabContent() {
          PusherView()
        }
        .tabBar(this.tabBuilder(3, $r('app.string.pusher')))

        TabContent() {
          SRView()
        }
        .tabBar(this.tabBuilder(4, $r('app.string.sr')))
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
        top: 0
      })
    }
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
    .backgroundColor('#F0F3F5')
  }
}
