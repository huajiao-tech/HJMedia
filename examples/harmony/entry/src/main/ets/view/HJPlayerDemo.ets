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

import { CommonConstant as Common } from '../common/CommonConstant';

@Component
export struct HJPlayerDemo {
  build() {
    Column() {
      Text('HJPlayer Demo')
        .fontSize(20)
        .fontWeight(Common.FONT_WEIGHT_500)
        .margin({ bottom: 16 })

      Button('进入 Player Demo')
        .fontSize(14)
        .height(36)
        .padding({ left: 16, right: 16 })
        .onClick(() => {
          this.getUIContext().getRouter().pushUrl({ url: 'pages/IndexV2' })
        })
    }
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .width(Common.FULL_PERCENT)
    .height(Common.FULL_PERCENT)
  }
}
