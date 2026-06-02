import { VideoCodecType } from 'hjpusher';

export class HJCommonDemoOption {
  faceuPath: string = 'faceu/60031_10';
  pngGiftPath: string = 'pngseq/ShuangDanCaiShen';

  constructor(faceuPath: string, pngGiftPath: string) {
    this.faceuPath = faceuPath;
    this.pngGiftPath = pngGiftPath;
  }
}

export class HJPusherDemoOption {
  url: string = '';
  codecID: number = VideoCodecType.HJCodecH264;
  recordUrl: string = '';

  constructor(url: string, codecID: number, recordUrl: string) {
    this.url = url;
    this.codecID = codecID;
    this.recordUrl = recordUrl;
  }
}

export const hjCommonDemoOption = new HJCommonDemoOption(
  'faceu/60031_10',
  'pngseq/ShuangDanCaiShen'
);

export const hjPusherDemoOption = new HJPusherDemoOption(
  'rtmp://live-push-0.test.huajiao.com/main/test?auth_key=1780381073-0-0-160369ec13e62754f23f206e519da246',
  VideoCodecType.HJCodecH264,
  '/data/storage/el2/base/haps/entry/files/HJPusherDemoRecord.mp4'
);
