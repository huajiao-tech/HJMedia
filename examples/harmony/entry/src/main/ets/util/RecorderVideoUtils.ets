import { fileIo } from "@kit.CoreFileKit";
import { photoAccessHelper } from "@kit.MediaLibraryKit";
import DateTimeUtil from "./DateTimeUtils";

export async function applySaveVideo(context: Context, path: string, src: string, ext: string) {
  const phAccessHelper = photoAccessHelper.getPhotoAccessHelper(context)
  try {
    const srcFileUri = path + "/" + src + "." + ext
    let srcFileUris: Array<string> = [
      srcFileUri
    ];
    let photoCreationConfigs: Array<photoAccessHelper.PhotoCreationConfig> = [
      {
        title: src, // 可选。
        fileNameExtension: ext,
        photoType: photoAccessHelper.PhotoType.VIDEO,
      }
    ];
    // 基于弹窗授权的方式获取媒体库的目标uri。
    const desFileUris: Array<string> = await phAccessHelper.showAssetsCreationDialog(srcFileUris, photoCreationConfigs);
    // 将来源于应用沙箱的照片内容写入媒体库的目标uri。
    let desFile: fileIo.File = await fileIo.open(desFileUris[0], fileIo.OpenMode.WRITE_ONLY);
    let srcFile: fileIo.File = await fileIo.open(srcFileUri, fileIo.OpenMode.READ_ONLY);
    await fileIo.copyFile(srcFile.fd, desFile.fd);
    fileIo.closeSync(srcFile);
    fileIo.closeSync(desFile);
    console.info('create asset by dialog successfully');
    return true
  } catch (err) {
    console.error(`failed to create asset by dialog successfully errCode is: ${err.code}, ${err.message}`);
    return false
  }
}