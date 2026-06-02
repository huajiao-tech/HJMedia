export class HJRteGraphConfig {
  // Node class names
  static HJNodeClass_SourceBridge: string = 'HJNodeClass_SourceBridge';
  static HJNodeClass_SourceBridgeMediaData: string = 'HJNodeClass_SourceBridgeMediaData';
  static HJNodeClass_SplitScreen: string = 'HJNodeClass_SplitScreen';
  static HJNodeClass_SplitScreenMediaData: string = 'HJNodeClass_SplitScreenMediaData';
  static HJNodeClass_SourcePNGSeq: string = 'HJNodeClass_SourcePNGSeq';
  static HJNodeClass_SourceFaceu: string = 'HJNodeClass_SourceFaceu';
  static HJNodeClass_SourceImage: string = 'HJNodeClass_SourceImage';
  static HJNodeClass_SourceImageSeq: string = 'HJNodeClass_SourceImageSeq';

  static HJNodeClass_CustomSourceFilter: string = 'HJNodeClass_CustomSourceFilter';
  static HJNodeClass_FilterBlur: string = 'HJNodeClass_FilterBlur';
  static HJNodeClass_FilterGray: string = 'HJNodeClass_FilterGray';
  static HJNodeClass_FilterDenoise: string = 'HJNodeClass_FilterDenoise';
  static HJNodeClass_FilterSR: string = 'HJNodeClass_FilterSR';
  static HJNodeClass_FilterCopy2D: string = 'HJNodeClass_FilterCopy2D';
  static HJNodeClass_FilterCopyOES: string = 'HJNodeClass_FilterCopyOES';

  static HJNodeClass_TargetUI_0: string = 'HJNodeClass_TargetUI_0';
  static HJNodeClass_TargetUI_1: string = 'HJNodeClass_TargetUI_1';
  static HJNodeClass_TargetUI_2: string = 'HJNodeClass_TargetUI_2';
  static HJNodeClass_TargetUI_3: string = 'HJNodeClass_TargetUI_3';
  static HJNodeClass_TargetEncoder: string = 'HJNodeClass_TargetEncoder';
  static HJNodeClass_TargetImgReceiver: string = 'HJNodeClass_TargetImgReceiver';
  static HJNodeClass_TargetPBODetect: string = 'HJNodeClass_TargetPBODetect';
  static HJNodeClass_TargetPBO_0: string = 'HJNodeClass_TargetPBO_0';
  static HJNodeClass_TargetPBO_1: string = 'HJNodeClass_TargetPBO_1';
  static HJNodeClass_TargetPBO_2: string = 'HJNodeClass_TargetPBO_2';
  static HJNodeClass_TargetPBO_3: string = 'HJNodeClass_TargetPBO_3';

  // Node roles
  static HJNodeRole_Source: string = 'HJNodeRole_Source';
  static HJNodeRole_Filter: string = 'HJNodeRole_Filter';
  static HJNodeRole_Target: string = 'HJNodeRole_Target';

  // Shader types
  static HJNodeShaderType_Copy2D: string = 'HJNodeShaderType_Copy2D';
  static HJNodeShaderType_CopyOES: string = 'HJNodeShaderType_CopyOES';
  static HJNodeShaderType_PreMul_Copy2D: string = 'HJNodeShaderType_PreMul_Copy2D';
  static HJNodeShaderType_PreMul_CopyOES: string = 'HJNodeShaderType_PreMul_CopyOES';
  static HJNodeShaderType_SplitScreenLR_2D: string = 'HJNodeShaderType_SplitScreenLR_2D';
  static HJNodeShaderType_SplitScreenLR_OES: string = 'HJNodeShaderType_SplitScreenLR_OES';

  // Render modes
  static HJNodeRenderType_Clip: string = 'HJNodeRenderType_Clip';
  static HJNodeRenderType_Fit: string = 'HJNodeRenderType_Fit';
  static HJNodeRenderType_Origin: string = 'HJNodeRenderType_Origin';
  static HJNodeRenderType_Full: string = 'HJNodeRenderType_Full';
}

export interface HJRteAdjacentNodeInfo {
  classStyle: string;
  insName: string;
}

export interface HJRteAdjacentNodeInfoWrapper {
  nodes: HJRteAdjacentNodeInfo[];
}

export interface HJRteJsonLinkInfoPlain {
  linkId: string;
  renderMode: string;
  x: number;
  y: number;
  w: number;
  h: number;
  xMirror: boolean;
  yMirror: boolean;
}

export class HJRteJsonLinkInfo {
  linkId: string = '';
  renderMode: string = '';
  x: number = 0.0;
  y: number = 0.0;
  w: number = 1.0;
  h: number = 1.0;
  xMirror: boolean = false;
  yMirror: boolean = false;

  constructor(init?: HJRteJsonLinkInfoPlain) {
    if (init !== undefined) {
      this.linkId = init.linkId;
      this.renderMode = init.renderMode;
      this.x = init.x;
      this.y = init.y;
      this.w = init.w;
      this.h = init.h;
      this.xMirror = init.xMirror;
      this.yMirror = init.yMirror;
    }
  }

  setLink(renderMode: string = HJRteGraphConfig.HJNodeRenderType_Clip, x: number = 0.0, y: number = 0.0,
    w: number = 1.0, h: number = 1.0, xMirror: boolean = false, yMirror: boolean = false): void {
    this.renderMode = renderMode;
    this.x = x;
    this.y = y;
    this.w = w;
    this.h = h;
    this.xMirror = xMirror;
    this.yMirror = yMirror;
  }
  setLinkId(linkId: string): void {
    this.linkId = linkId;
  }

  toPlain(): HJRteJsonLinkInfoPlain {
    return {
      linkId: this.linkId,
      renderMode: this.renderMode,
      x: this.x,
      y: this.y,
      w: this.w,
      h: this.h,
      xMirror: this.xMirror,
      yMirror: this.yMirror
    };
  }

  toJSON(): HJRteJsonLinkInfoPlain {
    return this.toPlain();
  }
}

export interface HJRteJsonNodePlain {
  classStyle: string;
  insName: string;
  role: string;
  enable: boolean;
  isMainSource: boolean;
  resourceUrl: string;
  dependsOn: string;
}

export class HJRteJsonNode {
  classStyle: string = '';
  insName: string = '';
  role: string = '';
  enable: boolean = true;
  isMainSource: boolean = false;
  resourceUrl: string = '';
  dependsOn: string = '';

  constructor(classStyle: string = '', insName: string = '', role: string = '', enable: boolean = true,
    isMainSource: boolean = false, resourceUrl: string = '', dependsOn: string = '') {
    this.classStyle = classStyle;
    this.insName = insName;
    this.role = role;
    this.enable = enable;
    this.isMainSource = isMainSource;
    this.resourceUrl = resourceUrl;
    this.dependsOn = dependsOn;
  }

  toPlain(): HJRteJsonNodePlain {
    return {
      classStyle: this.classStyle,
      insName: this.insName,
      role: this.role,
      enable: this.enable,
      isMainSource: this.isMainSource,
      resourceUrl: this.resourceUrl,
      dependsOn: this.dependsOn
    };
  }

  toJSON(): HJRteJsonNodePlain {
    return this.toPlain();
  }
}

export interface HJRteGraphSetupInfoPlain {
  nodes: HJRteJsonNodePlain[];
  links: HJRteJsonLinkPlain[];
}

export class HJRteGraphSetupInfo {
  nodes: HJRteJsonNode[] = [];
  links: HJRteJsonLink[] = [];

  addSource(node: HJRteJsonNode): void {
    this.nodes.push(node);
  }

  addLink(link: HJRteJsonLink): void {
    this.links.push(link);
  }

  toPlain(): HJRteGraphSetupInfoPlain {
    const mappedNodes: HJRteJsonNodePlain[] = [];
    const len = this.nodes.length;
    for (let i = 0; i < len; i++) {
      mappedNodes.push(this.nodes[i].toPlain());
    }
    const mappedLinks: HJRteJsonLinkPlain[] = [];
    const linkLen = this.links.length;
    for (let i = 0; i < linkLen; i++) {
      mappedLinks.push(this.links[i].toPlain());
    }
    return {
      nodes: mappedNodes,
      links: mappedLinks
    };
  }

  initSerial(): string {
    return JSON.stringify(this.toPlain());
  }

  static fromJson(json: string): HJRteGraphSetupInfo {
    const parsed: HJRteGraphSetupInfoPlain = JSON.parse(json) as HJRteGraphSetupInfoPlain;
    const graph = new HJRteGraphSetupInfo();
    const nodesList = parsed ? parsed.nodes : [];
    if (nodesList && nodesList.length !== undefined) {
      for (let i = 0; i < nodesList.length; i++) {
        graph.nodes.push(HJRteGraphSetupInfo.nodeFromPlain(nodesList[i]));
      }
    }
    const linksList = parsed ? parsed.links : [];
    if (linksList && linksList.length !== undefined) {
      for (let i = 0; i < linksList.length; i++) {
        graph.links.push(HJRteGraphSetupInfo.linkFromPlain(linksList[i]));
      }
    }
    return graph;
  }

  private static nodeFromPlain(rawNode: HJRteJsonNodePlain): HJRteJsonNode {
    const node = new HJRteJsonNode(
      rawNode.classStyle,
      rawNode.insName,
      rawNode.role,
      rawNode.enable,
      rawNode.isMainSource,
      rawNode.resourceUrl ?? '',
      rawNode.dependsOn ?? ''
    );
    return node;
  }

  private static linkFromPlain(rawLink: HJRteJsonLinkPlain): HJRteJsonLink {
    const link = new HJRteJsonLink(rawLink.from, rawLink.to, rawLink.shaderType);
    link.link = new HJRteJsonLinkInfo(rawLink.link);
    return link;
  }
}

export interface HJRteJsonLinkPlain {
  from: string;
  to: string;
  shaderType: string;
  link: HJRteJsonLinkInfoPlain;
}

export class HJRteJsonLink {
  from: string = '';
  to: string = '';
  shaderType: string = '';
  link: HJRteJsonLinkInfo = new HJRteJsonLinkInfo();

  constructor(from: string = '', to: string = '', shaderType: string = '') {
    this.from = from;
    this.to = to;
    this.shaderType = shaderType;
  }

  toPlain(): HJRteJsonLinkPlain {
    return {
      from: this.from,
      to: this.to,
      shaderType: this.shaderType,
      link: this.link.toPlain()
    };
  }

  toJSON(): HJRteJsonLinkPlain {
    return this.toPlain();
  }
}
