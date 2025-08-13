import { camera } from '@kit.CameraKit';

export class VideoConstants {
  /**
   * Index page Tag.
   */
  static readonly INDEX_TAG: string = 'INDEX';
  /**
   * Recorder page Tag.
   */
  static readonly RECORDER_TAG: string = 'RECORDER';
  /**
   * Default ID.
   */
  static readonly DEFAULT_ID: string = '-1';
  /**
   * Default time.
   */
  static readonly DEFAULT_TIME: string = '00:00';
  /**
   * PX.
   */
  static readonly PX: string = 'px';
  /**
   * Default value.
   */
  static readonly DEFAULT_VALUE: number = 0;
  /**
   * Default profile.
   */
  static readonly DEFAULT_PROFILE: camera.Profile = {
    format: camera.CameraFormat.CAMERA_FORMAT_YUV_420_SP,
    size: {
      width: 1280,
      height: 720
    }
  };
  /**
   * Video avc mime type.
   */
  static readonly MIME_VIDEO_AVC: string = 'video/avc';
  /**
   * Video hevc mime type.
   */
  static readonly MIME_VIDEO_HEVC: string = 'video/hevc';
  /**
   * Default width.
   */
  static readonly DEFAULT_WIDTH: number = 1920;
  /**
   * Default height.
   */
  static readonly DEFAULT_HEIGHT: number = 1080;
  /**
   * 4K video width.
   */
  static readonly VIDEO_WIDTH_4K: number = 3840;
  /**
   * 4K video height.
   */
  static readonly VIDEO_HEIGHT_4K: number = 2160;
  /**
   * 1080P video width.
   */
  static readonly VIDEO_WIDTH_1080P: number = 1920;
  /**
   * 1080P video height.
   */
  static readonly VIDEO_HEIGHT_1080P: number = 1080;
  /**
   * 720P video width.
   */
  static readonly VIDEO_WIDTH_720P: number = 1280;
  /**
   * 720P video height.
   */
  static readonly VIDEO_HEIGHT_720P: number = 720;
  /**
   * 10M bitrate.
   */
  static readonly BITRATE_VIDEO_10M: number = 10 * 1024 * 1024;
  /**
   * 20M bitrate.
   */
  static readonly BITRATE_VIDEO_20M: number = 20 * 1024 * 1024;
  /**
   * 30M bitrate.
   */
  static readonly BITRATE_VIDEO_30M: number = 30 * 1024 * 1024;
  /**
   * 30 FPS.
   */
  static readonly FRAMERATE_VIDEO_30FPS: number = 30;
  /**
   * 60 FPS.
   */
  static readonly FRAMERATE_VIDEO_60FPS: number = 60;
  /**
   * Duration.
   */
  static readonly DURATION: number = 2000;
  /**
   * The distance between toast dialog box and the bottom of screen.
   */
  static readonly BOTTOM: number = 80;
  /**
   * Default picker item height.
   */
  static readonly DEFAULT_PICKER_ITEM_HEIGHT: number = 30;
  /**
   * Selected text style font size.
   */
  static readonly SELECTED_TEXT_STYLE_FONT_SIZE: number = 15;
  /**
   * Video mime type.
   */
  static readonly VIDEO_MIMETYPE: string[] = ['HDRVivid', 'H264', 'H265'];
  /**
   * Video resolution.
   */
  static readonly VIDEO_RESOLUTION: string[] = ['4K', '1080P', '720P'];
  /**
   * Video framerate.
   */
  static readonly VIDEO_FRAMERATE: string[] = ['30Fps', '60Fps'];
  /**
   * Video recorderInfo.
   */
  static readonly RECORDER_INFO: string[][] = [
    VideoConstants.VIDEO_MIMETYPE, VideoConstants.VIDEO_RESOLUTION, VideoConstants.VIDEO_FRAMERATE
  ];
  /**
   * The number corresponding to true.
   */
  static readonly TRUE: number = 1;
  /**
   * The number corresponding to false.
   */
  static readonly FALSE: number = 0;
  /**
   * Min range.
   */
  static readonly MIN_RANGE: number = 1;
  /**
   * Max range.
   */
  static readonly MAX_RANGE: number = 30;
  /**
   * Full size.
   */
  static readonly FULL_SIZE: string = '100%';
}

export const CAMERA_POSITION = "cameraPosition"