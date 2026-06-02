import { HJRteGraphConfig, HJRteGraphSetupInfo, HJRteJsonNode, HJRteJsonLink, HJRteJsonLinkInfo } from './HJRteGraphSetupInfo';

export enum HJRteGraphConstructorType {
  HJRteGraphConstructorType_Test = -1,
  HJRteGraphConstructorType_None = 0,
  HJRteGraphConstructorType_PlaceHolder = 1,
  HJRteGraphConstructorType_SplictScreen = 2,
  HJRteGraphConstructorType_Image = 3,
  HJRteGraphConstructorType_ImageSeq = 4,
}

export interface HJRteGraphConstructorParam {
  type?: HJRteGraphConstructorType;
  useCustomFilter?: boolean;
  isMirror?: boolean;
  isEnableEnhance?: boolean;
  isEnableEnhanceMatch?: boolean;
  isUI0Clip?: boolean;
}

export class HJRteGraphConfigConstructor {
  static constructGraph(param?: HJRteGraphConstructorParam): string {
    const type: HJRteGraphConstructorType = param?.type ?? HJRteGraphConstructorType.HJRteGraphConstructorType_None;
    switch (type) {
      case HJRteGraphConstructorType.HJRteGraphConstructorType_PlaceHolder:
        return HJRteGraphConfigConstructor.constructPlaceHolder(param);
      case HJRteGraphConstructorType.HJRteGraphConstructorType_SplictScreen:
        return HJRteGraphConfigConstructor.constructSplictScreen(param);
      case HJRteGraphConstructorType.HJRteGraphConstructorType_Image:
        return HJRteGraphConfigConstructor.constructImage(param);
      case HJRteGraphConstructorType.HJRteGraphConstructorType_ImageSeq:
        return HJRteGraphConfigConstructor.constructImageSeq(param);
      case HJRteGraphConstructorType.HJRteGraphConstructorType_Test:
        return HJRteGraphConfigConstructor.constructTest(param);
      case HJRteGraphConstructorType.HJRteGraphConstructorType_None:
      default:
        return '';
    }
  }

  private static constructPlaceHolder(param?: HJRteGraphConstructorParam): string {
    const cfg = new HJRteGraphSetupInfo();
    const useCustomFilter = param?.useCustomFilter ?? false;
    const isEnableEnhance = param?.isEnableEnhance ?? false;
    const isEnableEnhanceMatch = param?.isEnableEnhanceMatch ?? false;
    const isUI0Clip = param?.isUI0Clip ?? false;

    const videoSourceName = HJRteGraphConfigConstructor.getDefaultSourceName();
    const videoSourceNode = HJRteGraphConfigConstructor.createJsonNode(cfg, videoSourceName, videoSourceName, HJRteGraphConfig.HJNodeRole_Source, true, true);

    const video2DName = HJRteGraphConfigConstructor.getCopyFilterName();
    const video2D = HJRteGraphConfigConstructor.createJsonNode(cfg, video2DName, video2DName, HJRteGraphConfig.HJNodeRole_Filter);
    HJRteGraphConfigConstructor.connectNodes(cfg, videoSourceNode, video2D, new HJRteJsonLinkInfo(), '');

    const detectPBO = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetPBODetect, HJRteGraphConfig.HJNodeClass_TargetPBODetect, HJRteGraphConfig.HJNodeRole_Target, false);
    HJRteGraphConfigConstructor.connectNodes(cfg, video2D, detectPBO, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    const customFilter = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_CustomSourceFilter, HJRteGraphConfig.HJNodeClass_CustomSourceFilter, HJRteGraphConfig.HJNodeRole_Filter, useCustomFilter);
    HJRteGraphConfigConstructor.connectNodes(cfg, video2D, customFilter, new HJRteJsonLinkInfo(), '');

    const blur = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_FilterBlur, HJRteGraphConfig.HJNodeClass_FilterBlur, HJRteGraphConfig.HJNodeRole_Filter, false);
    HJRteGraphConfigConstructor.connectNodes(cfg, customFilter, blur, new HJRteJsonLinkInfo(), '');

    const ui2D_0 = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeRole_Target, false);
    const info = new HJRteJsonLinkInfo();
    info.renderMode = isUI0Clip ? HJRteGraphConfig.HJNodeRenderType_Clip : HJRteGraphConfig.HJNodeRenderType_Fit;
    if (isEnableEnhance)
    {
      if (isEnableEnhanceMatch)
      {
        HJRteGraphConfigConstructor.connectNodes(cfg, blur, ui2D_0, info, HJRteGraphConfig.HJNodeShaderType_Copy2D);
      }
      const filterDenoise = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_FilterDenoise, HJRteGraphConfig.HJNodeClass_FilterDenoise, HJRteGraphConfig.HJNodeRole_Filter, true);
      HJRteGraphConfigConstructor.connectNodes(cfg, blur, filterDenoise, new HJRteJsonLinkInfo(), '');

      const filterSR = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_FilterSR, HJRteGraphConfig.HJNodeClass_FilterSR, HJRteGraphConfig.HJNodeRole_Filter, true);
      HJRteGraphConfigConstructor.connectNodes(cfg, filterDenoise, filterSR, new HJRteJsonLinkInfo(), '');

      HJRteGraphConfigConstructor.connectNodes(cfg, filterSR, ui2D_0, info, HJRteGraphConfig.HJNodeShaderType_Copy2D);

    }
    else
    {
      HJRteGraphConfigConstructor.connectNodes(cfg, blur, ui2D_0, info, HJRteGraphConfig.HJNodeShaderType_Copy2D);
    }

    const ui2D_1 = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetUI_1, HJRteGraphConfig.HJNodeClass_TargetUI_1, HJRteGraphConfig.HJNodeRole_Target, false);
    HJRteGraphConfigConstructor.connectNodes(cfg, blur, ui2D_1, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    const faceu = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeRole_Source, false);
    HJRteGraphConfigConstructor.setDependsOn(faceu, videoSourceNode.insName);
    HJRteGraphConfigConstructor.connectNodes(cfg, faceu, ui2D_0, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);
    HJRteGraphConfigConstructor.connectNodes(cfg, faceu, ui2D_1, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    const encoderTarget = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetEncoder, HJRteGraphConfig.HJNodeClass_TargetEncoder, HJRteGraphConfig.HJNodeRole_Target, false);
    HJRteGraphConfigConstructor.connectNodes(cfg, blur, encoderTarget, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);
    HJRteGraphConfigConstructor.connectNodes(cfg, faceu, encoderTarget, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    return cfg.initSerial();
  }

  private static constructSplictScreen(param?: HJRteGraphConstructorParam): string {
    const cfg = new HJRteGraphSetupInfo();
    const bSplitScreenXMirror = param?.isMirror ?? false;

    const videoSourceName = HJRteGraphConfigConstructor.getSplitScreenSourceName();
    const videoSourceNode = HJRteGraphConfigConstructor.createJsonNode(cfg, videoSourceName, videoSourceName, HJRteGraphConfig.HJNodeRole_Source, true, true);

    const shaderType = HJRteGraphConfigConstructor.getSplitScreenShader();
    const ui = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeRole_Target, false);
    const uiLink = new HJRteJsonLinkInfo();
    uiLink.xMirror = bSplitScreenXMirror;
    HJRteGraphConfigConstructor.connectNodes(cfg, videoSourceNode, ui, uiLink, shaderType);

    if (videoSourceName === HJRteGraphConfig.HJNodeClass_SplitScreenMediaData) {
      const targetPBO_0 = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetPBO_0, HJRteGraphConfig.HJNodeClass_TargetPBO_0, HJRteGraphConfig.HJNodeRole_Target, true);
      const targetLink = new HJRteJsonLinkInfo();
      targetLink.xMirror = bSplitScreenXMirror;
      HJRteGraphConfigConstructor.connectNodes(cfg, videoSourceNode, targetPBO_0, targetLink, shaderType);
    }

    return cfg.initSerial();
  }

  private static constructImageSeq(param?: HJRteGraphConstructorParam): string {
    const cfg = new HJRteGraphSetupInfo();

    const imageSeqNode = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_SourceImageSeq, HJRteGraphConfig.HJNodeClass_SourceImageSeq, HJRteGraphConfig.HJNodeRole_Source, true, true);
    const ui = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeRole_Target, false);
    const faceu = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeRole_Source, true);
    HJRteGraphConfigConstructor.setDependsOn(faceu, imageSeqNode.insName);
    HJRteGraphConfigConstructor.connectNodes(cfg, imageSeqNode, ui, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_PreMul_Copy2D);
    HJRteGraphConfigConstructor.connectNodes(cfg, faceu, ui, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    return cfg.initSerial();
  }

  private static constructImage(param?: HJRteGraphConstructorParam): string {
    const cfg = new HJRteGraphSetupInfo();

    const imageNode = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_SourceImage, HJRteGraphConfig.HJNodeClass_SourceImage, HJRteGraphConfig.HJNodeRole_Source, true, true);

    const ui = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeRole_Target, false);
    const faceu = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeRole_Source, false);
    HJRteGraphConfigConstructor.setDependsOn(faceu, imageNode.insName);
    HJRteGraphConfigConstructor.connectNodes(cfg, imageNode, ui, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_PreMul_Copy2D);
    HJRteGraphConfigConstructor.connectNodes(cfg, faceu, ui, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    return cfg.initSerial();
  }

  private static constructTest(param?: HJRteGraphConstructorParam): string {
    const cfg = new HJRteGraphSetupInfo();

    const videoSourceName = HJRteGraphConfigConstructor.getDefaultSourceName();
    const videoSourceNode = HJRteGraphConfigConstructor.createJsonNode(cfg, videoSourceName, videoSourceName, HJRteGraphConfig.HJNodeRole_Source, true, true);

    const video2DName = HJRteGraphConfigConstructor.getCopyFilterName();
    const video2D = HJRteGraphConfigConstructor.createJsonNode(cfg, video2DName, video2DName, HJRteGraphConfig.HJNodeRole_Filter);
    HJRteGraphConfigConstructor.connectNodes(cfg, videoSourceNode, video2D, new HJRteJsonLinkInfo(), '');

    const ui2D = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeClass_TargetUI_0, HJRteGraphConfig.HJNodeRole_Target, false);
    HJRteGraphConfigConstructor.connectNodes(cfg, video2D, ui2D, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    const faceu = HJRteGraphConfigConstructor.createJsonNode(cfg, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeClass_SourceFaceu, HJRteGraphConfig.HJNodeRole_Source, false);
    HJRteGraphConfigConstructor.setDependsOn(faceu, videoSourceNode.insName);
    HJRteGraphConfigConstructor.connectNodes(cfg, faceu, ui2D, new HJRteJsonLinkInfo(), HJRteGraphConfig.HJNodeShaderType_Copy2D);

    return cfg.initSerial();
  }

  private static createJsonNode(cfg: HJRteGraphSetupInfo, classStyle: string, insName: string, role: string, enable: boolean = true, isMainSource: boolean = false): HJRteJsonNode {
    const node = new HJRteJsonNode(classStyle, insName, role, enable, isMainSource);
    cfg.addSource(node);
    return node;
  }

  private static connectNodes(cfg: HJRteGraphSetupInfo, src: HJRteJsonNode, dst: HJRteJsonNode, linkInfo: HJRteJsonLinkInfo, shaderType: string) {
    const link = new HJRteJsonLink(src.insName, dst.insName, shaderType);
    link.link = linkInfo;
    cfg.addLink(link);
  }

  private static setDependsOn(node: HJRteJsonNode, sourceInsName: string): void {
    node.dependsOn = sourceInsName;
  }

  private static getDefaultSourceName(): string {
    return HJRteGraphConfig.HJNodeClass_SourceBridge;
  }

  private static getSplitScreenSourceName(): string {
    return HJRteGraphConfig.HJNodeClass_SplitScreen;
  }

  private static getCopyFilterName(): string {
    return HJRteGraphConfig.HJNodeClass_FilterCopyOES;
  }

  private static getSplitScreenShader(): string {
    return HJRteGraphConfig.HJNodeShaderType_SplitScreenLR_OES;
  }
}
