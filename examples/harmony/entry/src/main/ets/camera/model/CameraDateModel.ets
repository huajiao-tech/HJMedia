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

import { camera } from '@kit.CameraKit';
import { VideoConstants as Const } from '../common/CommonConstants';

export class CameraDataModel {
  public surfaceId: string = '';
  public cameraWidth: number = Const.VIDEO_WIDTH_720P;
  public cameraHeight: number = Const.VIDEO_HEIGHT_720P;
  public isHDRVivid: number = Const.DEFAULT_VALUE;
  public outputfd: number = -1;
  public frameRate: number = Const.FRAMERATE_VIDEO_30FPS;
  public previewProfile: camera.Profile = Const.DEFAULT_PROFILE;
  public videoCodecMime: string | null = Const.MIME_VIDEO_AVC;
  public bitRate: number = Const.BITRATE_VIDEO_20M;
  public cameraPosition: number = 0;

  setCodecFormat(isHDR: number, codecMime: string): void {
    this.isHDRVivid = isHDR;
    this.videoCodecMime = codecMime;
  }

  setResolution(width: number, height: number, bit: number): void {
    this.cameraWidth = width;
    this.cameraHeight = height;
    this.bitRate = bit;
  }
}