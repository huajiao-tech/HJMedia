import { camera } from '@kit.CameraKit';
import { BusinessError } from '@kit.BasicServicesKit';
import Logger from './Logger';
import { VideoConstants as Const } from '../CommonConstants';
import { colorSpaceManager } from '@kit.ArkGraphics2D';

const TAG: string = Const.RECORDER_TAG;

export function isVideoStabilizationModeSupported(session: camera.VideoSession, mode: camera.VideoStabilizationMode): boolean {
  let isSupported: boolean = false;
  try {
    isSupported = session.isVideoStabilizationModeSupported(mode);
  } catch (error) {
    // Failed to return error code error. code and handle it.
    let err = error as BusinessError;
    Logger.error(TAG, `The isVideoStabilizationModeSupported call failed. error code: ${err.code}`);
  }
  return isSupported;
}

export function setVideoStabilizationMode(session: camera.VideoSession): boolean {
  let mode: camera.VideoStabilizationMode = camera.VideoStabilizationMode.AUTO;
  // Check if video stabilization is supported.
  let isSupported: boolean = isVideoStabilizationModeSupported(session, mode);
  if (isSupported) {
    Logger.info(TAG, `setVideoStabilizationMode: ${mode}`);
    session.setVideoStabilizationMode(mode);
    let activeVideoStabilizationMode = session.getActiveVideoStabilizationMode();
    Logger.info(TAG, `activeVideoStabilizationMode: ${activeVideoStabilizationMode}`);
  } else {
    Logger.info(TAG, `videoStabilizationMode: ${mode} is not support`);
  }
  return isSupported;
}

export function getSupportedColorSpaces(session: camera.VideoSession): Array<colorSpaceManager.ColorSpace> {
  let colorSpaces: Array<colorSpaceManager.ColorSpace> = [];
  try {
    colorSpaces = session.getSupportedColorSpaces();
  } catch (error) {
    let err = error as BusinessError;
    Logger.error(TAG, `The getSupportedColorSpaces call failed. error code: ${err.code}`);
  }
  return colorSpaces;
}

export function setColorSpaceBeforeCommitConfig(session: camera.VideoSession, isHdr: number): void {
  let colorSpace: colorSpaceManager.ColorSpace = isHdr? colorSpaceManager.ColorSpace.BT2020_HLG_LIMIT : colorSpaceManager.ColorSpace.BT709_LIMIT;
  let colorSpaces: Array<colorSpaceManager.ColorSpace> = getSupportedColorSpaces(session);
  let isSupportedColorSpaces = colorSpaces.indexOf(colorSpace) >= 0;
  if (isSupportedColorSpaces) {
    Logger.info(TAG, `setColorSpace: ${colorSpace}`);
    session.setColorSpace(colorSpace);
    let activeColorSpace:colorSpaceManager.ColorSpace = session.getActiveColorSpace();
    Logger.info(TAG, `activeColorSpace: ${activeColorSpace}`);
  } else {
    Logger.info(TAG, `colorSpace: ${colorSpace} is not support`);
  }
}
