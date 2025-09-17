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
import Logger from './Logger';
import { CameraDataModel } from '../../model/CameraDateModel';
import { VideoConstants as Const } from '../CommonConstants';

const TAG = 'CAMERA_CHECK';

function getPreviewProfile(previewProfiles: Array<camera.Profile>,
  cameraData: CameraDataModel): undefined | camera.Profile {
  let previewProfile: undefined | camera.Profile = previewProfiles.find((profile: camera.Profile) => {
    return profile.format === cameraData.previewProfile.format &&
      profile.size.width === cameraData.previewProfile.size.width && profile.size.height === cameraData.previewProfile.size.height
  });
  Logger.info(TAG, `profile.format: ${JSON.stringify(previewProfile)}`);
  return previewProfile;
}

// export function previewProfileCameraCheck(cameraManager: camera.CameraManager,
//   cameraData: CameraDataModel): undefined | camera.Profile {
//   let cameraDevices = cameraManager.getSupportedCameras();
//   if (cameraDevices !== undefined && cameraDevices.length <= 0) {
//     Logger.error(TAG, 'cameraManager.getSupportedCameras error!');
//     return;
//   }
//
//   let profiles: camera.CameraOutputCapability =
//     cameraManager.getSupportedOutputCapability(cameraDevices[cameraData.cameraPosition], camera.SceneMode.NORMAL_VIDEO);
//   if (!profiles) {
//     Logger.error(TAG, 'cameraManager.getSupportedOutputCapability error!');
//     return;
//   }
//
//   let previewProfilesArray: Array<camera.Profile> = profiles.previewProfiles;
//   if (!previewProfilesArray) {
//     Logger.error(TAG, "createOutput previewProfilesArray == null || undefined");
//     return;
//   }
//
//   let videoSize: camera.Size = {
//     width: 1920,
//     height: 1080
//   }
//
//   let previewProfile: undefined | camera.Profile = getPreviewProfile(previewProfilesArray, videoSize,
//       cameraData.isHDRVivid);
//   if (!previewProfile) {
//     Logger.error(TAG, 'previewProfile is not found');
//     return;
//   }
//   return previewProfile;
// }

// export function videoProfileCheck(cameraManager: camera.CameraManager,
//   cameraData: CameraDataModel): undefined | camera.VideoProfile {
//   let cameraDevices = cameraManager.getSupportedCameras();
//   if (cameraDevices !== undefined && cameraDevices.length <= 0) {
//     Logger.error(TAG, 'cameraManager.getSupportedCameras error!');
//     return;
//   }
//
//   let profiles: camera.CameraOutputCapability =
//     cameraManager.getSupportedOutputCapability(cameraDevices[0], camera.SceneMode.NORMAL_VIDEO);
//   if (!profiles) {
//     Logger.error(TAG, 'cameraManager.getSupportedOutputCapability error!');
//     return;
//   }
//
//   let videoProfiles: Array<camera.VideoProfile> = profiles.videoProfiles;
//   if (!videoProfiles) {
//     Logger.error(TAG, 'Get videoProfiles error!');
//     return;
//   }
//
//   let videoProfile: undefined | camera.VideoProfile = videoProfiles.find((profile: camera.VideoProfile) => {
//     if (cameraData.isHDRVivid) {
//       if (cameraData.frameRate === Const.FRAMERATE_VIDEO_30FPS) {
//         return profile.size.width === cameraData.cameraWidth &&
//           profile.size.height === cameraData.cameraHeight &&
//           profile.format === camera.CameraFormat.CAMERA_FORMAT_YCBCR_P010 &&
//           profile.frameRateRange.min === 1 &&
//           profile.frameRateRange.max === 30;
//       } else {
//         return profile.size.width === cameraData.cameraWidth &&
//           profile.size.height === cameraData.cameraHeight &&
//           profile.format === camera.CameraFormat.CAMERA_FORMAT_YCBCR_P010 &&
//           profile.frameRateRange.min === cameraData.frameRate &&
//           profile.frameRateRange.max === cameraData.frameRate;
//       }
//     } else {
//       if (cameraData.frameRate === Const.FRAMERATE_VIDEO_30FPS) {
//         return profile.size.width === cameraData.cameraWidth &&
//           profile.size.height === cameraData.cameraHeight &&
//           profile.format === camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP &&
//           profile.frameRateRange.min === 1 &&
//           profile.frameRateRange.max === 30;
//       } else {
//         return profile.size.width === cameraData.cameraWidth &&
//           profile.size.height === cameraData.cameraHeight &&
//           profile.format === camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP &&
//           profile.frameRateRange.min === cameraData.frameRate &&
//           profile.frameRateRange.max === cameraData.frameRate;
//       }
//     }
//   });
//   return videoProfile;
// }

export function profileCameraCheck(cameraManager: camera.CameraManager,
  cameraData: CameraDataModel, cameraDevice: camera.CameraDevice): undefined | camera.Profile {
  let cameraOutputCapability: camera.CameraOutputCapability =
    cameraManager.getSupportedOutputCapability(cameraDevice, camera.SceneMode.NORMAL_VIDEO);
  if (!cameraOutputCapability) {
    Logger.error(TAG, 'cameraManager.getSupportedOutputCapability error!');
    return;
  }

  let previewProfiles: Array<camera.Profile> = cameraOutputCapability.previewProfiles;
  if (!previewProfiles) {
    Logger.error(TAG, "createOutput previewProfiles == null || undefined");
    return;
  }

  let previewProfile: undefined | camera.Profile = getPreviewProfile(previewProfiles, cameraData);
  if (!previewProfile) {
    Logger.error(TAG, 'previewProfile is not found');
    return;
  }
  return previewProfile;
}