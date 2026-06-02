#import "SRViewController.h"
#import "HJSRGLView.h"

#import <QuartzCore/CADisplayLink.h>

#include "HJCommonInterface.h"
#include "HJConvertUtils.h"
#include "HJFaceDetectExport.h"
#include "HJInferenceContextExport.h"
#include "HJTransferMediaData.h"
#include "HJVideoSRExport.h"
#include "utils/HJFaceSRWrapper.h"
#include "utils/HJImageSeqWrapper.h"
#include <cstdio>

NS_HJ_USING

static const int kSRFPS = 15;
static NSString *const kModelRealESRGAN = @"realesrgan";
static NSString *const kModelRealCUGAN = @"realcugan";
static NSString *const kInputSourceImageSeq = @"imageseq";
static NSString *const kInputSourceImage = @"image";
static NSString *const kDefaultInputFolder = @"360x640";
static NSString *const kDefaultImageName = @"4.jpg";
static NSString *const kCoreMLModelDefault = @"realesr-general-merge05-x4v3-fixed-200x356-image-ios16";
static NSString *const kCoreMLFaceScaleModelDefault = @"realesr-general-merge05-x4v3-fixed-160x200-image-ios16";
static NSString *const kCoreMLFullScaleFolder = @"model/CoreML/realesr/fullScale";
static NSString *const kCoreMLFaceScaleFolder = @"model/CoreML/realesr/faceScale";
static NSString *const kSRModeFull = @"Full";
static NSString *const kSRModeFace = @"Face";
static NSString *const kSRModeFaceScale = @"FaceScale";
static NSString *const kSRModeFullScale = @"FullScale";

static CGSize HJClipOutputSize(CGSize targetSize, CGSize srcSize) {
    if (targetSize.width <= 0.0 || targetSize.height <= 0.0 || srcSize.width <= 0.0 || srcSize.height <= 0.0) {
        return CGSizeZero;
    }
    CGFloat dw = targetSize.width;
    CGFloat dh = targetSize.height;
    const CGFloat width = srcSize.width;
    const CGFloat height = srcSize.height;
    if (dw < (dh * width / height)) {
        dw = dh * width / height;
    } else {
        dh = dw * height / width;
    }
    return CGSizeMake(MAX(1.0, dw), MAX(1.0, dh));
}

static CGSize HJComposeCanvasSize(CGSize drawableSize, CGFloat displayScaleW, CGFloat displayScaleH, int inputWidth, int inputHeight) {
    if (drawableSize.width <= 0.0 || drawableSize.height <= 0.0 || inputWidth <= 0 || inputHeight <= 0) {
        return CGSizeZero;
    }
    const int targetWidth = MAX(1, (int)lrint(drawableSize.width));
    const int targetHeight = MAX(1, (int)lrint(drawableSize.height));
    const int displayWidth = MAX(1, MIN(targetWidth, (int)lrint((double)targetWidth * displayScaleW)));
    const int displayHeight = MAX(1, MIN(targetHeight, (int)lrint((double)targetHeight * displayScaleH)));
    const CGSize fboSize = HJClipOutputSize(CGSizeMake(displayWidth, displayHeight), CGSizeMake(inputWidth, inputHeight));
    return CGSizeMake(MAX(1.0, (CGFloat)lrint(fboSize.width)), MAX(1.0, (CGFloat)lrint(fboSize.height)));
}

typedef NS_ENUM(NSInteger, HJCoreMLComputeMode) {
    HJCoreMLComputeModeAll = 0,
    HJCoreMLComputeModeCPUAndGPU = 1,
    HJCoreMLComputeModeCPUAndNeuralEngine = 2,
    HJCoreMLComputeModeCPUOnly = 3,
};

typedef NS_ENUM(NSInteger, HJCoreMLScaleMode) {
    HJCoreMLScaleModeFullScale = 0,
    HJCoreMLScaleModeFaceScale = 1,
};

@interface SRSetting : NSObject
@property (nonatomic, assign) BOOL useGPU;
@property (nonatomic, assign) BOOL useCoreML;
@property (nonatomic, assign) BOOL useVTProcessor;
@property (nonatomic, assign) NSInteger coreMLComputeMode;
@property (nonatomic, assign) NSInteger coreMLScaleMode;
@property (nonatomic, copy) NSString *coreMLModelName;
@property (nonatomic, copy) NSString *coreMLFullScaleModelName;
@property (nonatomic, copy) NSString *coreMLFaceScaleModelName;
@property (nonatomic, copy) NSString *model;
@property (nonatomic, copy) NSString *variant;
@property (nonatomic, assign) NSInteger threadNums;
@property (nonatomic, copy) NSString *inputSource;
@property (nonatomic, copy) NSString *inputFolder;
@property (nonatomic, copy) NSString *imageFile;
@end

@implementation SRSetting
@end

@interface SRViewController ()
@property (nonatomic, strong) HJSRGLView *glPreviewView;
@property (nonatomic, strong) UILabel *infoLabel;
@property (nonatomic, strong) UILabel *mixLabel;
@property (nonatomic, strong) UILabel *matchLabel;
@property (nonatomic, strong) UILabel *displayScaleWLabel;
@property (nonatomic, strong) UILabel *displayScaleHLabel;
@property (nonatomic, strong) UILabel *preScaleLabel;
@property (nonatomic, strong) UILabel *faceSRStatusLabel;
@property (nonatomic, strong) UILabel *postResizeLabel;
@property (nonatomic, strong) UILabel *settingLabel;
@property (nonatomic, strong) UISwitch *faceSRSwitch;
@property (nonatomic, strong) UISwitch *postResizeSwitch;
@property (nonatomic, strong) UISlider *mixSlider;
@property (nonatomic, strong) UISlider *matchSlider;
@property (nonatomic, strong) UISlider *displayScaleWSlider;
@property (nonatomic, strong) UISlider *displayScaleHSlider;
@property (nonatomic, strong) UIButton *startButton;
@property (nonatomic, strong) UIButton *pauseButton;
@property (nonatomic, strong) UIButton *settingButton;
@property (nonatomic, strong) UIButton *modeButton;
@property (nonatomic, strong) UIButton *policyButton;
@property (nonatomic, strong) UIButton *preScaleButton;
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, strong) dispatch_queue_t processQueue;
@property (nonatomic, assign) BOOL frameProcessing;
@property (nonatomic, assign) BOOL controlTracking;
@property (nonatomic, assign) BOOL started;
@property (nonatomic, assign) BOOL paused;
@property (nonatomic, assign) BOOL inferenceContextReady;
@property (nonatomic, assign) NSInteger tickCounter;
@property (atomic, assign) NSUInteger renderRevision;
@property (nonatomic, assign) NSInteger srMode;
@property (nonatomic, assign) NSInteger procPolicyIndex;
@property (nonatomic, assign) NSInteger faceScalePresetIndex;
@property (nonatomic, assign) NSInteger fullScalePresetIndex;
@property (nonatomic, assign) NSInteger lastFaceTargetDisplayW;
@property (nonatomic, assign) NSInteger lastFaceTargetDisplayH;
@property (nonatomic, assign) NSInteger lastSROutputW;
@property (nonatomic, assign) NSInteger lastSROutputH;
@property (nonatomic, assign) float displayScaleW;
@property (nonatomic, assign) float displayScaleH;
@property (nonatomic, strong) SRSetting *setting;
@property (nonatomic, strong) NSArray<NSString *> *esrganVariants;
@property (nonatomic, strong) NSArray<NSString *> *cuganVariants;
@property (nonatomic, strong) NSArray<NSNumber *> *threadOptions;
@property (nonatomic, strong) NSArray<NSString *> *inputFolderOptions;
@property (nonatomic, strong) NSArray<NSString *> *imageFileOptions;
@property (nonatomic, strong) NSArray<NSString *> *srModeOptions;
@property (nonatomic, strong) NSArray<NSString *> *procPolicyOptions;
@property (nonatomic, strong) NSArray<NSString *> *faceScalePresetLabels;
@property (nonatomic, strong) NSArray<NSValue *> *faceScalePresetSizes;
@property (nonatomic, strong) NSArray<NSString *> *fullScalePresetLabels;
@property (nonatomic, strong) NSArray<NSValue *> *fullScalePresetSizes;
@property (nonatomic, strong) UIImage *singleImage;
@property (nonatomic, weak) UIViewController *settingDialogVC;
@property (nonatomic, weak) UISegmentedControl *settingDeviceSeg;
@property (nonatomic, weak) UISegmentedControl *settingModelSeg;
@property (nonatomic, weak) UISegmentedControl *settingVariantSeg;
@property (nonatomic, weak) UISegmentedControl *settingThreadSeg;
@property (nonatomic, weak) UISegmentedControl *settingBackendSeg;
@property (nonatomic, weak) UISegmentedControl *settingCoreMLUnitSeg;
@property (nonatomic, weak) UISegmentedControl *settingCoreMLScaleSeg;
@property (nonatomic, weak) UIButton *settingSourceButton;
@property (nonatomic, weak) UIButton *settingCoreMLModelButton;
@property (nonatomic, weak) UIButton *settingFolderButton;
@property (nonatomic, weak) UIButton *settingImageButton;
@property (nonatomic, weak) UIStackView *settingFolderRow;
@property (nonatomic, weak) UIStackView *settingImageRow;
@property (nonatomic, weak) UIStackView *settingCoreMLUnitRow;
@property (nonatomic, weak) UIStackView *settingCoreMLScaleRow;
@property (nonatomic, weak) UIStackView *settingCoreMLModelRow;
@end

@implementation SRViewController
{
    std::unique_ptr<HJImageSeqWrapper> _imageSeqWrapper;
    std::unique_ptr<HJFaceSRWrapper> _faceSRWrapper;
}

- (UIColor *)uiGreenColor {
    return [UIColor systemGreenColor];
}

- (void)applyOverlayTextStyleToLabel:(UILabel *)label {
    if (!label) {
        return;
    }
    label.textColor = [UIColor colorWithWhite:0.98 alpha:1.0];
    label.shadowColor = [[UIColor blackColor] colorWithAlphaComponent:0.75];
    label.shadowOffset = CGSizeMake(0.0, 1.0);
}

- (void)applyOverlayAccentStyleToButton:(UIButton *)button {
    if (!button) {
        return;
    }
    UIColor *accentColor = [UIColor colorWithRed:0.68 green:1.0 blue:0.78 alpha:1.0];
    [button setTitleColor:accentColor forState:UIControlStateNormal];
    button.tintColor = accentColor;
    button.titleLabel.font = [UIFont systemFontOfSize:15.0 weight:UIFontWeightSemibold];
    button.layer.shadowColor = [UIColor.blackColor colorWithAlphaComponent:0.7].CGColor;
    button.layer.shadowOpacity = 1.0;
    button.layer.shadowRadius = 1.0;
    button.layer.shadowOffset = CGSizeMake(0.0, 1.0);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"SR超分";
    self.view.backgroundColor = [UIColor systemBackgroundColor];
    [self setupDefaults];
    [self setupUI];
    [self updateMixLabel];
    [self updateMatchLabel];
    [self updateDisplayScaleLabels];
    [self updatePreScaleLabel];
    [self updateSettingLabel];
    [self refreshFaceSRSwitchState];
    [self updatePauseButtonTitle];

    [self initInferenceContextIfNeeded];
    [self startProcess];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self stopProcess];
}

- (void)dealloc {
    [self stopProcess];
    [self releaseInferenceContextIfNeeded];
}

- (void)setupDefaults {
    self.processQueue = dispatch_queue_create("com.hjmedia.ios.srface.process", DISPATCH_QUEUE_SERIAL);
    self.esrganVariants = @[ @"realesr-general-x4v3", @"realesr-animevideov3-x2" ];
    self.cuganVariants = @[ @"conservative", @"no-denoise" ];
    self.threadOptions = @[ @1, @2, @4, @6 ];
    self.srModeOptions = @[ kSRModeFull, kSRModeFace, kSRModeFaceScale, kSRModeFullScale ];
    self.procPolicyOptions = @[ @"Mipmap", @"Bilinear", @"Bicubic", @"Lanczos3", @"Lanczos4" ];
    self.faceScalePresetLabels = @[ @"80x100", @"90x114", @"100x126", @"180x228", @"220x278", @"300x380", @"360x456" ];
    self.faceScalePresetSizes = @[
        [NSValue valueWithCGSize:CGSizeMake(80, 100)],
        [NSValue valueWithCGSize:CGSizeMake(90, 114)],
        [NSValue valueWithCGSize:CGSizeMake(100, 126)],
        [NSValue valueWithCGSize:CGSizeMake(180, 228)],
        [NSValue valueWithCGSize:CGSizeMake(220, 278)],
        [NSValue valueWithCGSize:CGSizeMake(300, 380)],
        [NSValue valueWithCGSize:CGSizeMake(360, 456)]
    ];
    self.fullScalePresetLabels = @[ @"90x160", @"180x320", @"200x356", @"224x398", @"240x426", @"260x462", @"280x498", @"300x534", @"320x568", @"360x640" ];
    self.fullScalePresetSizes = @[
        [NSValue valueWithCGSize:CGSizeMake(90, 160)],
        [NSValue valueWithCGSize:CGSizeMake(180, 320)],
        [NSValue valueWithCGSize:CGSizeMake(200, 356)],
        [NSValue valueWithCGSize:CGSizeMake(224, 398)],
        [NSValue valueWithCGSize:CGSizeMake(240, 426)],
        [NSValue valueWithCGSize:CGSizeMake(260, 462)],
        [NSValue valueWithCGSize:CGSizeMake(280, 498)],
        [NSValue valueWithCGSize:CGSizeMake(300, 534)],
        [NSValue valueWithCGSize:CGSizeMake(320, 568)],
        [NSValue valueWithCGSize:CGSizeMake(360, 640)]
    ];
    NSArray<NSValue *> *scannedFullScaleSizes = [self scanFullScalePresetSizesFromCoreMLModels];
    if (scannedFullScaleSizes.count > 0) {
        self.fullScalePresetSizes = scannedFullScaleSizes;
        NSMutableArray<NSString *> *labels = [NSMutableArray arrayWithCapacity:scannedFullScaleSizes.count];
        for (NSValue *value in scannedFullScaleSizes) {
            CGSize size = value.CGSizeValue;
            [labels addObject:[NSString stringWithFormat:@"%dx%d", (int)size.width, (int)size.height]];
        }
        self.fullScalePresetLabels = labels;
    }
    NSArray<NSValue *> *scannedFaceScaleSizes = [self scanFaceScalePresetSizesFromCoreMLModels];
    if (scannedFaceScaleSizes.count > 0) {
        self.faceScalePresetSizes = scannedFaceScaleSizes;
        NSMutableArray<NSString *> *labels = [NSMutableArray arrayWithCapacity:scannedFaceScaleSizes.count];
        for (NSValue *value in scannedFaceScaleSizes) {
            CGSize size = value.CGSizeValue;
            [labels addObject:[NSString stringWithFormat:@"%dx%d", (int)size.width, (int)size.height]];
        }
        self.faceScalePresetLabels = labels;
    }
    self.inputFolderOptions = [self scanInputFolders];
    self.imageFileOptions = @[];
    self.srMode = HJFaceSRProcessMode_FaceScale;
    self.procPolicyIndex = 3;
    NSInteger defaultFaceScaleIndex = [self indexOfSize:CGSizeMake(160, 200) inArray:self.faceScalePresetSizes];
    self.faceScalePresetIndex = (defaultFaceScaleIndex == NSNotFound) ? 0 : defaultFaceScaleIndex;
    NSInteger defaultFullScaleIndex = [self indexOfSize:CGSizeMake(200, 356) inArray:self.fullScalePresetSizes];
    self.fullScalePresetIndex = (defaultFullScaleIndex == NSNotFound) ? 0 : defaultFullScaleIndex;
    self.displayScaleW = 1.0f;
    self.displayScaleH = 1.0f;

    self.setting = [[SRSetting alloc] init];
    self.setting.useGPU = YES;
    self.setting.useCoreML = YES;
    self.setting.useVTProcessor = NO;
    self.setting.coreMLComputeMode = HJCoreMLComputeModeAll;
    self.setting.coreMLScaleMode = HJCoreMLScaleModeFaceScale;
    self.setting.coreMLModelName = kCoreMLFaceScaleModelDefault;
    self.setting.coreMLFullScaleModelName = kCoreMLModelDefault;
    self.setting.coreMLFaceScaleModelName = kCoreMLFaceScaleModelDefault;
    self.setting.model = kModelRealESRGAN;
    self.setting.variant = self.esrganVariants.firstObject;
    self.setting.threadNums = self.setting.useGPU ? 1 : 2;
    self.setting.inputSource = kInputSourceImageSeq;
    self.setting.inputFolder = self.inputFolderOptions.firstObject ?: kDefaultInputFolder;
    self.setting.imageFile = kDefaultImageName;
}

- (UILabel *)makeInfoLabelWithFont:(UIFont *)font color:(UIColor *)color {
    UILabel *label = [[UILabel alloc] init];
    label.translatesAutoresizingMaskIntoConstraints = NO;
    label.font = font;
    label.textColor = color;
    label.numberOfLines = 0;
    [self applyOverlayTextStyleToLabel:label];
    return label;
}

- (UIButton *)makeMenuButton {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    button.showsMenuAsPrimaryAction = YES;
    button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentRight;
    [self applyOverlayAccentStyleToButton:button];
    return button;
}

- (UIButton *)makeSettingMenuButton {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    button.showsMenuAsPrimaryAction = YES;
    button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentRight;
    button.titleLabel.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold];
    [button setTitleColor:[UIColor systemBlueColor] forState:UIControlStateNormal];
    return button;
}

- (void)configureSettingMenuButton:(UIButton *)button
                            titles:(NSArray<NSString *> *)titles
                     selectedIndex:(NSInteger)selectedIndex
                           handler:(void (^)(NSInteger idx))handler {
    if (!button) {
        return;
    }
    NSInteger safeIndex = selectedIndex;
    if (safeIndex < 0 || safeIndex >= (NSInteger)titles.count) {
        safeIndex = 0;
    }
    NSString *selectedTitle = titles.count > 0 ? titles[safeIndex] : @"-";
    [button setTitle:selectedTitle forState:UIControlStateNormal];
    button.tag = safeIndex;

    NSMutableArray<UIMenuElement *> *actions = [NSMutableArray array];
    [titles enumerateObjectsUsingBlock:^(NSString * _Nonnull title, NSUInteger idx, BOOL * _Nonnull stop) {
        UIAction *action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
            button.tag = (NSInteger)idx;
            [button setTitle:title forState:UIControlStateNormal];
            if (handler) {
                handler((NSInteger)idx);
            }
        }];
        action.state = (safeIndex == (NSInteger)idx) ? UIMenuElementStateOn : UIMenuElementStateOff;
        [actions addObject:action];
    }];
    button.menu = [UIMenu menuWithChildren:actions];
}

- (BOOL)isFaceMode:(NSInteger)srMode {
    return srMode == HJFaceSRProcessMode_FaceOrigin || srMode == HJFaceSRProcessMode_FaceScale;
}

- (BOOL)usesPreScaleMode:(NSInteger)srMode {
    return srMode == HJFaceSRProcessMode_FaceScale || srMode == HJFaceSRProcessMode_FullScale;
}

- (NSArray<NSString *> *)currentPreScalePresetLabels {
    if (self.srMode == HJFaceSRProcessMode_FaceScale) {
        return self.faceScalePresetLabels;
    }
    if (self.srMode == HJFaceSRProcessMode_FullScale) {
        return self.fullScalePresetLabels;
    }
    return @[];
}

- (NSArray<NSValue *> *)currentPreScalePresetSizes {
    if (self.srMode == HJFaceSRProcessMode_FaceScale) {
        return self.faceScalePresetSizes;
    }
    if (self.srMode == HJFaceSRProcessMode_FullScale) {
        return self.fullScalePresetSizes;
    }
    return @[];
}

- (CGSize)resolvedPreScaleSize {
    NSArray<NSValue *> *sizes = [self currentPreScalePresetSizes];
    if (sizes.count == 0) {
        return CGSizeZero;
    }
    NSInteger index = (self.srMode == HJFaceSRProcessMode_FaceScale) ? self.faceScalePresetIndex : self.fullScalePresetIndex;
    index = MAX(0, MIN(index, (NSInteger)sizes.count - 1));
    return [sizes[index] CGSizeValue];
}

- (NSString *)coreMLModelNameForCurrentSRMode {
    if (self.setting.coreMLScaleMode == HJCoreMLScaleModeFaceScale) {
        return self.setting.coreMLFaceScaleModelName ?: @"";
    }
    return self.setting.coreMLFullScaleModelName ?: @"";
}

- (NSArray<NSString *> *)selectedCoreMLModelOptions {
    return (self.setting.coreMLScaleMode == HJCoreMLScaleModeFaceScale)
        ? [self coreMLFaceScaleModelOptions]
        : [self coreMLFullScaleModelOptions];
}

- (NSArray<NSString *> *)coreMLModelOptionsForScaleMode:(NSInteger)scaleMode {
    return (scaleMode == HJCoreMLScaleModeFaceScale)
        ? [self coreMLFaceScaleModelOptions]
        : [self coreMLFullScaleModelOptions];
}

- (NSString *)selectedCoreMLModelNameFromSetting {
    return (self.setting.coreMLScaleMode == HJCoreMLScaleModeFaceScale)
        ? (self.setting.coreMLFaceScaleModelName ?: @"")
        : (self.setting.coreMLFullScaleModelName ?: @"");
}

- (NSString *)coreMLModelNameFromSettingForScaleMode:(NSInteger)scaleMode {
    return (scaleMode == HJCoreMLScaleModeFaceScale)
        ? (self.setting.coreMLFaceScaleModelName ?: @"")
        : (self.setting.coreMLFullScaleModelName ?: @"");
}

- (void)applyCoreMLSettingToModeAndPreScale {
    if (!self.setting.useCoreML) {
        return;
    }

    NSString *modelName = [self selectedCoreMLModelNameFromSetting];
    if (modelName.length <= 0) {
        return;
    }
    CGSize requiredSize = [self requiredPreScaleSizeForCoreMLModelName:modelName];
    if (CGSizeEqualToSize(requiredSize, CGSizeZero)) {
        return;
    }

    self.setting.coreMLModelName = modelName;
    if (self.setting.coreMLScaleMode == HJCoreMLScaleModeFaceScale) {
        NSInteger idx = [self indexOfSize:requiredSize inArray:self.faceScalePresetSizes];
        if (idx != NSNotFound) {
            self.srMode = HJFaceSRProcessMode_FaceScale;
            self.faceScalePresetIndex = idx;
        }
    } else {
        NSInteger idx = [self indexOfSize:requiredSize inArray:self.fullScalePresetSizes];
        if (idx != NSNotFound) {
            self.srMode = HJFaceSRProcessMode_FullScale;
            self.fullScalePresetIndex = idx;
        }
    }
}

- (int)selectedProcPolicy {
    if (![self usesPreScaleMode:self.srMode]) {
        return HJFaceSRProcessPolicy_Bilinear;
    }
    switch (self.procPolicyIndex) {
        case 0: return HJFaceSRProcessPolicy_Mipmap;
        case 1: return HJFaceSRProcessPolicy_Bilinear;
        case 2: return HJFaceSRProcessPolicy_Bicubic;
        case 3: return HJFaceSRProcessPolicy_Lanczos3;
        case 4: return HJFaceSRProcessPolicy_Lanczos4;
        default: return HJFaceSRProcessPolicy_Mipmap;
    }
}

- (void)updatePauseButtonTitle {
    [self.pauseButton setTitle:(self.paused ? @"恢复" : @"暂停") forState:UIControlStateNormal];
}

- (void)updateMixLabel {
    self.mixLabel.text = [NSString stringWithFormat:@"%.2f", self.mixSlider.value];
}

- (void)updateMatchLabel {
    self.matchLabel.text = [NSString stringWithFormat:@"%.2f", self.matchSlider.value];
}

- (void)updateDisplayScaleLabels {
    self.displayScaleWLabel.text = [NSString stringWithFormat:@"%.2f", self.displayScaleWSlider.value];
    self.displayScaleHLabel.text = [NSString stringWithFormat:@"%.2f", self.displayScaleHSlider.value];
}

- (void)updatePreScaleLabel {
    if (![self usesPreScaleMode:self.srMode]) {
        self.preScaleLabel.text = @"PreScale: Off";
        return;
    }
    CGSize size = [self resolvedPreScaleSize];
    self.preScaleLabel.text = [NSString stringWithFormat:@"PreScale: %dx%d", (int)size.width, (int)size.height];
}

- (NSString *)postResizeModeText {
    if (![self isFaceMode:self.srMode]) {
        return @"face-only";
    }
    return self.postResizeSwitch.on ? @"nativeLanczos" : @"glLinear";
}

- (void)updatePostResizeLabel {
    const BOOL faceMode = [self isFaceMode:self.srMode];
    self.postResizeSwitch.enabled = faceMode;
    self.postResizeSwitch.alpha = faceMode ? 1.0 : 0.45;
    if (!faceMode) {
        self.postResizeLabel.text = @"PostResize: Face-only";
        return;
    }
    NSString *targetText = @"";
    if (self.lastFaceTargetDisplayW > 0 && self.lastFaceTargetDisplayH > 0) {
        targetText = [NSString stringWithFormat:@" / target:%ldx%ld",
                      (long)self.lastFaceTargetDisplayW,
                      (long)self.lastFaceTargetDisplayH];
    }
    NSString *srOutputText = @"";
    if (self.lastSROutputW > 0 && self.lastSROutputH > 0) {
        srOutputText = [NSString stringWithFormat:@" / srOut:%ldx%ld",
                        (long)self.lastSROutputW,
                        (long)self.lastSROutputH];
    }
    self.postResizeLabel.text = [NSString stringWithFormat:@"PostResize: %@%@%@",
                                 [self postResizeModeText],
                                 targetText,
                                 srOutputText];
}

- (void)updateFaceSRStatusLabel {
    self.faceSRStatusLabel.text = self.faceSRSwitch.on ? @"on" : @"off";
    self.faceSRStatusLabel.textColor = [self uiGreenColor];
}

- (void)refreshSRControlMenus {
    SRViewController *controller = self;
    const BOOL coreMLLocked = self.setting.useCoreML;

    NSMutableArray<UIMenuElement *> *modeActions = [NSMutableArray array];
    [self.srModeOptions enumerateObjectsUsingBlock:^(NSString * _Nonnull title, NSUInteger idx, BOOL * _Nonnull stop) {
        UIAction *action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
            controller.srMode = (NSInteger)idx;
            [controller bumpRenderRevision];
            [controller refreshSRControlMenus];
            [controller syncPreviewRenderState];
        }];
        action.state = (controller.srMode == (NSInteger)idx ? UIMenuElementStateOn : UIMenuElementStateOff);
        [modeActions addObject:action];
    }];
    [self.modeButton setTitle:self.srModeOptions[MAX(0, MIN(self.srMode, (NSInteger)self.srModeOptions.count - 1))] forState:UIControlStateNormal];
    self.modeButton.menu = [UIMenu menuWithChildren:modeActions];
    self.modeButton.enabled = !coreMLLocked;
    self.modeButton.alpha = coreMLLocked ? 0.5 : 1.0;

    NSMutableArray<UIMenuElement *> *policyActions = [NSMutableArray array];
    [self.procPolicyOptions enumerateObjectsUsingBlock:^(NSString * _Nonnull title, NSUInteger idx, BOOL * _Nonnull stop) {
        UIAction *action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
            controller.procPolicyIndex = (NSInteger)idx;
            [controller bumpRenderRevision];
            [controller refreshSRControlMenus];
            [controller syncPreviewRenderState];
        }];
        action.state = (controller.procPolicyIndex == (NSInteger)idx ? UIMenuElementStateOn : UIMenuElementStateOff);
        [policyActions addObject:action];
    }];
    [self.policyButton setTitle:self.procPolicyOptions[MAX(0, MIN(self.procPolicyIndex, (NSInteger)self.procPolicyOptions.count - 1))] forState:UIControlStateNormal];
    self.policyButton.menu = [UIMenu menuWithChildren:policyActions];

    NSArray<NSString *> *preScaleLabels = [self currentPreScalePresetLabels];
    NSMutableArray<UIMenuElement *> *preScaleActions = [NSMutableArray array];
    [preScaleLabels enumerateObjectsUsingBlock:^(NSString * _Nonnull title, NSUInteger idx, BOOL * _Nonnull stop) {
        BOOL selected = (controller.srMode == HJFaceSRProcessMode_FaceScale)
            ? (controller.faceScalePresetIndex == (NSInteger)idx)
            : (controller.fullScalePresetIndex == (NSInteger)idx);
        UIAction *action = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
            if (controller.srMode == HJFaceSRProcessMode_FaceScale) {
                controller.faceScalePresetIndex = (NSInteger)idx;
            } else if (controller.srMode == HJFaceSRProcessMode_FullScale) {
                controller.fullScalePresetIndex = (NSInteger)idx;
            }
            [controller bumpRenderRevision];
            [controller refreshSRControlMenus];
            [controller syncPreviewRenderState];
        }];
        action.state = (selected ? UIMenuElementStateOn : UIMenuElementStateOff);
        [preScaleActions addObject:action];
    }];
    NSString *preScaleTitle = preScaleLabels.count > 0 ? preScaleLabels[((self.srMode == HJFaceSRProcessMode_FaceScale) ? MAX(0, MIN(self.faceScalePresetIndex, (NSInteger)preScaleLabels.count - 1)) : MAX(0, MIN(self.fullScalePresetIndex, (NSInteger)preScaleLabels.count - 1)))] : @"Off";
    [self.preScaleButton setTitle:preScaleTitle forState:UIControlStateNormal];
    self.preScaleButton.menu = [UIMenu menuWithChildren:preScaleActions];
    self.preScaleButton.enabled = (preScaleLabels.count > 0) && !coreMLLocked;
    self.preScaleButton.alpha = self.preScaleButton.enabled ? 1.0 : 0.5;
    self.policyButton.enabled = [self usesPreScaleMode:self.srMode];
    self.policyButton.alpha = self.policyButton.enabled ? 1.0 : 0.5;
    [self updatePreScaleLabel];
    [self updatePostResizeLabel];
    [self updateSettingLabel];
}

- (void)setupUI {
    UIView *preview = [[UIView alloc] init];
    preview.translatesAutoresizingMaskIntoConstraints = NO;
    preview.backgroundColor = [UIColor blackColor];
    preview.layer.cornerRadius = 8.0;
    preview.layer.masksToBounds = YES;

    self.glPreviewView = [[HJSRGLView alloc] init];
    self.glPreviewView.translatesAutoresizingMaskIntoConstraints = NO;
    self.glPreviewView.backgroundColor = [UIColor blackColor];
    [preview addSubview:self.glPreviewView];

    self.startButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.startButton.translatesAutoresizingMaskIntoConstraints = NO;
    [self.startButton setTitle:@"停止" forState:UIControlStateNormal];
    [self applyOverlayAccentStyleToButton:self.startButton];
    [self.startButton addTarget:self action:@selector(onStartStopTapped) forControlEvents:UIControlEventTouchUpInside];

    self.pauseButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.pauseButton.translatesAutoresizingMaskIntoConstraints = NO;
    [self applyOverlayAccentStyleToButton:self.pauseButton];
    [self.pauseButton addTarget:self action:@selector(onPauseResumeTapped) forControlEvents:UIControlEventTouchUpInside];

    self.settingButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.settingButton.translatesAutoresizingMaskIntoConstraints = NO;
    [self.settingButton setTitle:@"设置" forState:UIControlStateNormal];
    [self applyOverlayAccentStyleToButton:self.settingButton];
    [self.settingButton addTarget:self action:@selector(onSettingTapped:) forControlEvents:UIControlEventTouchUpInside];
    [self.settingButton addTarget:self action:@selector(onSettingTouchDown:) forControlEvents:UIControlEventTouchDown];

    self.faceSRSwitch = [[UISwitch alloc] init];
    self.faceSRSwitch.translatesAutoresizingMaskIntoConstraints = NO;
    self.faceSRSwitch.on = YES;
    [self.faceSRSwitch addTarget:self action:@selector(onFaceSRToggled:) forControlEvents:UIControlEventValueChanged];

    self.postResizeSwitch = [[UISwitch alloc] init];
    self.postResizeSwitch.translatesAutoresizingMaskIntoConstraints = NO;
    self.postResizeSwitch.on = NO;
    [self.postResizeSwitch addTarget:self action:@selector(onPostResizeToggled:) forControlEvents:UIControlEventValueChanged];

    UILabel *headerFaceSRTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:13.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    headerFaceSRTitle.text = @"FaceSR";
    [self applyOverlayTextStyleToLabel:headerFaceSRTitle];
    UIStackView *headerFaceSRItem = [[UIStackView alloc] initWithArrangedSubviews:@[ headerFaceSRTitle, self.faceSRSwitch ]];
    headerFaceSRItem.translatesAutoresizingMaskIntoConstraints = NO;
    headerFaceSRItem.axis = UILayoutConstraintAxisHorizontal;
    headerFaceSRItem.alignment = UIStackViewAlignmentCenter;
    headerFaceSRItem.spacing = 6.0;

    UIStackView *headerButtons = [[UIStackView alloc] initWithArrangedSubviews:@[ self.startButton, self.pauseButton, self.settingButton, headerFaceSRItem ]];
    headerButtons.translatesAutoresizingMaskIntoConstraints = NO;
    headerButtons.axis = UILayoutConstraintAxisHorizontal;
    headerButtons.spacing = 10.0;
    headerButtons.distribution = UIStackViewDistributionFill;
    headerButtons.userInteractionEnabled = YES;

    self.infoLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];
    self.mixLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];
    self.matchLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];
    self.preScaleLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];
    self.displayScaleWLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];
    self.displayScaleHLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];
    self.faceSRStatusLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:28.0 weight:UIFontWeightBold] color:[self uiGreenColor]];
    self.postResizeLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];
    self.faceSRStatusLabel.textAlignment = NSTextAlignmentRight;
    [self updateFaceSRStatusLabel];
    self.settingLabel = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:12.0] color:[self uiGreenColor]];

    self.mixSlider = [[UISlider alloc] init];
    self.mixSlider.translatesAutoresizingMaskIntoConstraints = NO;
    self.mixSlider.minimumValue = 0.0f;
    self.mixSlider.maximumValue = 1.0f;
    self.mixSlider.value = 0.8f;
    [self.mixSlider addTarget:self action:@selector(onMixChanged:) forControlEvents:UIControlEventValueChanged];
    [self.mixSlider addTarget:self action:@selector(onControlTouchDown:) forControlEvents:UIControlEventTouchDown];
    [self.mixSlider addTarget:self action:@selector(onControlTouchUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
    UIView *mixSliderContainer = [[UIView alloc] init];
    mixSliderContainer.translatesAutoresizingMaskIntoConstraints = NO;
    [mixSliderContainer addSubview:self.mixSlider];
    [NSLayoutConstraint activateConstraints:@[
        [mixSliderContainer.heightAnchor constraintEqualToConstant:30.0],
        [self.mixSlider.centerXAnchor constraintEqualToAnchor:mixSliderContainer.centerXAnchor],
        [self.mixSlider.centerYAnchor constraintEqualToAnchor:mixSliderContainer.centerYAnchor],
        [self.mixSlider.widthAnchor constraintEqualToAnchor:mixSliderContainer.widthAnchor multiplier:0.72]
    ]];

    self.matchSlider = [[UISlider alloc] init];
    self.matchSlider.translatesAutoresizingMaskIntoConstraints = NO;
    self.matchSlider.minimumValue = 0.0f;
    self.matchSlider.maximumValue = 1.0f;
    self.matchSlider.value = 1.0f;
    [self.matchSlider addTarget:self action:@selector(onMatchChanged:) forControlEvents:UIControlEventValueChanged];
    [self.matchSlider addTarget:self action:@selector(onControlTouchDown:) forControlEvents:UIControlEventTouchDown];
    [self.matchSlider addTarget:self action:@selector(onControlTouchUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
    UIView *matchSliderContainer = [[UIView alloc] init];
    matchSliderContainer.translatesAutoresizingMaskIntoConstraints = NO;
    [matchSliderContainer addSubview:self.matchSlider];
    [NSLayoutConstraint activateConstraints:@[
        [matchSliderContainer.heightAnchor constraintEqualToConstant:30.0],
        [self.matchSlider.centerXAnchor constraintEqualToAnchor:matchSliderContainer.centerXAnchor],
        [self.matchSlider.centerYAnchor constraintEqualToAnchor:matchSliderContainer.centerYAnchor],
        [self.matchSlider.widthAnchor constraintEqualToAnchor:matchSliderContainer.widthAnchor multiplier:0.72]
    ]];

    self.displayScaleWSlider = [[UISlider alloc] init];
    self.displayScaleWSlider.translatesAutoresizingMaskIntoConstraints = NO;
    self.displayScaleWSlider.minimumValue = 0.0f;
    self.displayScaleWSlider.maximumValue = 1.0f;
    self.displayScaleWSlider.value = self.displayScaleW;
    [self.displayScaleWSlider addTarget:self action:@selector(onDisplayScaleWChanged:) forControlEvents:UIControlEventValueChanged];
    [self.displayScaleWSlider addTarget:self action:@selector(onControlTouchDown:) forControlEvents:UIControlEventTouchDown];
    [self.displayScaleWSlider addTarget:self action:@selector(onControlTouchUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
    UIView *displayScaleWSliderContainer = [[UIView alloc] init];
    displayScaleWSliderContainer.translatesAutoresizingMaskIntoConstraints = NO;
    [displayScaleWSliderContainer addSubview:self.displayScaleWSlider];
    [NSLayoutConstraint activateConstraints:@[
        [displayScaleWSliderContainer.heightAnchor constraintEqualToConstant:30.0],
        [self.displayScaleWSlider.centerXAnchor constraintEqualToAnchor:displayScaleWSliderContainer.centerXAnchor],
        [self.displayScaleWSlider.centerYAnchor constraintEqualToAnchor:displayScaleWSliderContainer.centerYAnchor],
        [self.displayScaleWSlider.widthAnchor constraintEqualToAnchor:displayScaleWSliderContainer.widthAnchor multiplier:0.72]
    ]];

    self.displayScaleHSlider = [[UISlider alloc] init];
    self.displayScaleHSlider.translatesAutoresizingMaskIntoConstraints = NO;
    self.displayScaleHSlider.minimumValue = 0.0f;
    self.displayScaleHSlider.maximumValue = 1.0f;
    self.displayScaleHSlider.value = self.displayScaleH;
    [self.displayScaleHSlider addTarget:self action:@selector(onDisplayScaleHChanged:) forControlEvents:UIControlEventValueChanged];
    [self.displayScaleHSlider addTarget:self action:@selector(onControlTouchDown:) forControlEvents:UIControlEventTouchDown];
    [self.displayScaleHSlider addTarget:self action:@selector(onControlTouchUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
    UIView *displayScaleHSliderContainer = [[UIView alloc] init];
    displayScaleHSliderContainer.translatesAutoresizingMaskIntoConstraints = NO;
    [displayScaleHSliderContainer addSubview:self.displayScaleHSlider];
    [NSLayoutConstraint activateConstraints:@[
        [displayScaleHSliderContainer.heightAnchor constraintEqualToConstant:30.0],
        [self.displayScaleHSlider.centerXAnchor constraintEqualToAnchor:displayScaleHSliderContainer.centerXAnchor],
        [self.displayScaleHSlider.centerYAnchor constraintEqualToAnchor:displayScaleHSliderContainer.centerYAnchor],
        [self.displayScaleHSlider.widthAnchor constraintEqualToAnchor:displayScaleHSliderContainer.widthAnchor multiplier:0.72]
    ]];

    UILabel *mixTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    mixTitle.text = @"FaceSR Mix";
    UILabel *matchTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    matchTitle.text = @"FaceSR Match";
    UILabel *modeTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    modeTitle.text = @"Mode";
    UILabel *policyTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    policyTitle.text = @"Policy";
    UILabel *preScaleTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    preScaleTitle.text = @"PreScale";
    UILabel *postResizeTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    postResizeTitle.text = @"PostResize";
    UILabel *displayScaleWTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    displayScaleWTitle.text = @"Display W";
    UILabel *displayScaleHTitle = [self makeInfoLabelWithFont:[UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold] color:[UIColor labelColor]];
    displayScaleHTitle.text = @"Display H";
    [self applyOverlayTextStyleToLabel:mixTitle];
    [self applyOverlayTextStyleToLabel:matchTitle];
    [self applyOverlayTextStyleToLabel:modeTitle];
    [self applyOverlayTextStyleToLabel:policyTitle];
    [self applyOverlayTextStyleToLabel:preScaleTitle];
    [self applyOverlayTextStyleToLabel:postResizeTitle];
    [self applyOverlayTextStyleToLabel:displayScaleWTitle];
    [self applyOverlayTextStyleToLabel:displayScaleHTitle];

    self.modeButton = [self makeMenuButton];
    self.policyButton = [self makeMenuButton];
    self.preScaleButton = [self makeMenuButton];

    [self applyOverlayTextStyleToLabel:self.mixLabel];
    [self applyOverlayTextStyleToLabel:self.matchLabel];
    [self applyOverlayTextStyleToLabel:self.displayScaleWLabel];
    [self applyOverlayTextStyleToLabel:self.displayScaleHLabel];
    [self applyOverlayTextStyleToLabel:self.preScaleLabel];
    [self applyOverlayTextStyleToLabel:self.postResizeLabel];
    UIStackView *mixRow = [[UIStackView alloc] initWithArrangedSubviews:@[ mixTitle, self.mixLabel ]];
    mixRow.translatesAutoresizingMaskIntoConstraints = NO;
    mixRow.axis = UILayoutConstraintAxisHorizontal;
    mixRow.distribution = UIStackViewDistributionEqualSpacing;
    mixRow.alignment = UIStackViewAlignmentCenter;

    UIStackView *matchRow = [[UIStackView alloc] initWithArrangedSubviews:@[ matchTitle, self.matchLabel ]];
    matchRow.translatesAutoresizingMaskIntoConstraints = NO;
    matchRow.axis = UILayoutConstraintAxisHorizontal;
    matchRow.distribution = UIStackViewDistributionEqualSpacing;
    matchRow.alignment = UIStackViewAlignmentCenter;

    UIStackView *sliderTitleRow = [[UIStackView alloc] initWithArrangedSubviews:@[ mixRow, matchRow ]];
    sliderTitleRow.translatesAutoresizingMaskIntoConstraints = NO;
    sliderTitleRow.axis = UILayoutConstraintAxisHorizontal;
    sliderTitleRow.distribution = UIStackViewDistributionFillEqually;
    sliderTitleRow.alignment = UIStackViewAlignmentFill;
    sliderTitleRow.spacing = 8.0;

    UIStackView *sliderPairRow = [[UIStackView alloc] initWithArrangedSubviews:@[ mixSliderContainer, matchSliderContainer ]];
    sliderPairRow.translatesAutoresizingMaskIntoConstraints = NO;
    sliderPairRow.axis = UILayoutConstraintAxisHorizontal;
    sliderPairRow.distribution = UIStackViewDistributionFillEqually;
    sliderPairRow.alignment = UIStackViewAlignmentCenter;
    sliderPairRow.spacing = 8.0;

    UIStackView *displayScaleWRow = [[UIStackView alloc] initWithArrangedSubviews:@[ displayScaleWTitle, self.displayScaleWLabel ]];
    displayScaleWRow.translatesAutoresizingMaskIntoConstraints = NO;
    displayScaleWRow.axis = UILayoutConstraintAxisHorizontal;
    displayScaleWRow.distribution = UIStackViewDistributionEqualSpacing;
    displayScaleWRow.alignment = UIStackViewAlignmentCenter;

    UIStackView *displayScaleHRow = [[UIStackView alloc] initWithArrangedSubviews:@[ displayScaleHTitle, self.displayScaleHLabel ]];
    displayScaleHRow.translatesAutoresizingMaskIntoConstraints = NO;
    displayScaleHRow.axis = UILayoutConstraintAxisHorizontal;
    displayScaleHRow.distribution = UIStackViewDistributionEqualSpacing;
    displayScaleHRow.alignment = UIStackViewAlignmentCenter;

    UIStackView *displayScaleTitleRow = [[UIStackView alloc] initWithArrangedSubviews:@[ displayScaleWRow, displayScaleHRow ]];
    displayScaleTitleRow.translatesAutoresizingMaskIntoConstraints = NO;
    displayScaleTitleRow.axis = UILayoutConstraintAxisHorizontal;
    displayScaleTitleRow.distribution = UIStackViewDistributionFillEqually;
    displayScaleTitleRow.alignment = UIStackViewAlignmentFill;
    displayScaleTitleRow.spacing = 8.0;

    UIStackView *displayScaleSliderRow = [[UIStackView alloc] initWithArrangedSubviews:@[ displayScaleWSliderContainer, displayScaleHSliderContainer ]];
    displayScaleSliderRow.translatesAutoresizingMaskIntoConstraints = NO;
    displayScaleSliderRow.axis = UILayoutConstraintAxisHorizontal;
    displayScaleSliderRow.distribution = UIStackViewDistributionFillEqually;
    displayScaleSliderRow.alignment = UIStackViewAlignmentCenter;
    displayScaleSliderRow.spacing = 8.0;

    UIStackView *modeRow = [[UIStackView alloc] initWithArrangedSubviews:@[ modeTitle, self.modeButton ]];
    modeRow.translatesAutoresizingMaskIntoConstraints = NO;
    modeRow.axis = UILayoutConstraintAxisHorizontal;
    modeRow.alignment = UIStackViewAlignmentCenter;
    modeRow.distribution = UIStackViewDistributionEqualSpacing;

    UIStackView *policyRow = [[UIStackView alloc] initWithArrangedSubviews:@[ policyTitle, self.policyButton ]];
    policyRow.translatesAutoresizingMaskIntoConstraints = NO;
    policyRow.axis = UILayoutConstraintAxisHorizontal;
    policyRow.alignment = UIStackViewAlignmentCenter;
    policyRow.distribution = UIStackViewDistributionEqualSpacing;

    UIStackView *preScaleRow = [[UIStackView alloc] initWithArrangedSubviews:@[ preScaleTitle, self.preScaleButton ]];
    preScaleRow.translatesAutoresizingMaskIntoConstraints = NO;
    preScaleRow.axis = UILayoutConstraintAxisHorizontal;
    preScaleRow.alignment = UIStackViewAlignmentCenter;
    preScaleRow.distribution = UIStackViewDistributionEqualSpacing;

    UIStackView *preScaleInfoRow = [[UIStackView alloc] initWithArrangedSubviews:@[ self.preScaleLabel ]];
    preScaleInfoRow.translatesAutoresizingMaskIntoConstraints = NO;
    preScaleInfoRow.axis = UILayoutConstraintAxisHorizontal;
    preScaleInfoRow.alignment = UIStackViewAlignmentCenter;

    UIStackView *postResizeAccessory = [[UIStackView alloc] initWithArrangedSubviews:@[ self.postResizeLabel, self.postResizeSwitch ]];
    postResizeAccessory.translatesAutoresizingMaskIntoConstraints = NO;
    postResizeAccessory.axis = UILayoutConstraintAxisHorizontal;
    postResizeAccessory.alignment = UIStackViewAlignmentCenter;
    postResizeAccessory.spacing = 8.0;
    [postResizeAccessory setContentCompressionResistancePriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
    [postResizeAccessory setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];

    UIView *postResizeSpacer = [[UIView alloc] init];
    postResizeSpacer.translatesAutoresizingMaskIntoConstraints = NO;

    UIStackView *postResizeRow = [[UIStackView alloc] initWithArrangedSubviews:@[ postResizeTitle, postResizeAccessory, postResizeSpacer ]];
    postResizeRow.translatesAutoresizingMaskIntoConstraints = NO;
    postResizeRow.axis = UILayoutConstraintAxisHorizontal;
    postResizeRow.alignment = UIStackViewAlignmentCenter;
    postResizeRow.distribution = UIStackViewDistributionFill;
    postResizeRow.spacing = 12.0;

    UIStackView *overlayPanel = [[UIStackView alloc] initWithArrangedSubviews:@[
        headerButtons,
        self.infoLabel,
        modeRow,
        policyRow,
        preScaleRow,
        preScaleInfoRow,
        postResizeRow,
        sliderTitleRow,
        sliderPairRow,
        displayScaleTitleRow,
        displayScaleSliderRow,
        self.settingLabel
    ]];
    overlayPanel.translatesAutoresizingMaskIntoConstraints = NO;
    overlayPanel.axis = UILayoutConstraintAxisVertical;
    overlayPanel.spacing = 8.0;
    overlayPanel.alignment = UIStackViewAlignmentFill;

    UIView *overlayCard = [[UIView alloc] init];
    overlayCard.translatesAutoresizingMaskIntoConstraints = NO;
    overlayCard.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.28];
    overlayCard.layer.cornerRadius = 12.0;
    overlayCard.layer.masksToBounds = YES;
    [overlayCard addSubview:overlayPanel];

    UIScrollView *overlayScrollView = [[UIScrollView alloc] init];
    overlayScrollView.translatesAutoresizingMaskIntoConstraints = NO;
    overlayScrollView.alwaysBounceVertical = YES;
    overlayScrollView.showsVerticalScrollIndicator = YES;
    overlayScrollView.backgroundColor = [UIColor clearColor];
    [overlayScrollView addSubview:overlayCard];
    [preview addSubview:overlayScrollView];
    [preview addSubview:self.faceSRStatusLabel];

    UIStackView *root = [[UIStackView alloc] initWithArrangedSubviews:@[ preview ]];
    root.translatesAutoresizingMaskIntoConstraints = NO;
    root.axis = UILayoutConstraintAxisVertical;
    root.spacing = 0.0;
    root.alignment = UIStackViewAlignmentFill;

    UIScrollView *scrollView = [[UIScrollView alloc] init];
    scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    scrollView.alwaysBounceVertical = YES;
    [self.view addSubview:scrollView];
    [scrollView addSubview:root];

    UILayoutGuide *guide = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [scrollView.leadingAnchor constraintEqualToAnchor:guide.leadingAnchor constant:12.0],
        [scrollView.trailingAnchor constraintEqualToAnchor:guide.trailingAnchor constant:-12.0],
        [scrollView.topAnchor constraintEqualToAnchor:guide.topAnchor constant:8.0],
        [scrollView.bottomAnchor constraintEqualToAnchor:guide.bottomAnchor constant:-8.0],

        [root.leadingAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.leadingAnchor],
        [root.trailingAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.trailingAnchor],
        [root.topAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.topAnchor],
        [root.bottomAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.bottomAnchor],
        [root.widthAnchor constraintEqualToAnchor:scrollView.frameLayoutGuide.widthAnchor],

        [preview.heightAnchor constraintEqualToAnchor:preview.widthAnchor multiplier:16.0/9.0],

        [self.glPreviewView.leadingAnchor constraintEqualToAnchor:preview.leadingAnchor],
        [self.glPreviewView.trailingAnchor constraintEqualToAnchor:preview.trailingAnchor],
        [self.glPreviewView.topAnchor constraintEqualToAnchor:preview.topAnchor],
        [self.glPreviewView.bottomAnchor constraintEqualToAnchor:preview.bottomAnchor],

        [self.faceSRStatusLabel.topAnchor constraintEqualToAnchor:preview.topAnchor constant:12.0],
        [self.faceSRStatusLabel.trailingAnchor constraintEqualToAnchor:preview.trailingAnchor constant:-12.0],

        [overlayScrollView.leadingAnchor constraintEqualToAnchor:preview.leadingAnchor constant:8.0],
        [overlayScrollView.trailingAnchor constraintEqualToAnchor:preview.trailingAnchor constant:-8.0],
        [overlayScrollView.bottomAnchor constraintEqualToAnchor:preview.bottomAnchor constant:-8.0],
        [overlayScrollView.heightAnchor constraintEqualToAnchor:preview.heightAnchor multiplier:0.36],
        [overlayScrollView.heightAnchor constraintGreaterThanOrEqualToConstant:180.0],
        [overlayScrollView.heightAnchor constraintLessThanOrEqualToConstant:260.0],

        [overlayCard.leadingAnchor constraintEqualToAnchor:overlayScrollView.contentLayoutGuide.leadingAnchor],
        [overlayCard.trailingAnchor constraintEqualToAnchor:overlayScrollView.contentLayoutGuide.trailingAnchor],
        [overlayCard.topAnchor constraintEqualToAnchor:overlayScrollView.contentLayoutGuide.topAnchor],
        [overlayCard.bottomAnchor constraintEqualToAnchor:overlayScrollView.contentLayoutGuide.bottomAnchor],
        [overlayCard.widthAnchor constraintEqualToAnchor:overlayScrollView.frameLayoutGuide.widthAnchor],

        [overlayPanel.leadingAnchor constraintEqualToAnchor:overlayCard.leadingAnchor constant:10.0],
        [overlayPanel.trailingAnchor constraintEqualToAnchor:overlayCard.trailingAnchor constant:-10.0],
        [overlayPanel.topAnchor constraintEqualToAnchor:overlayCard.topAnchor constant:10.0],
        [overlayPanel.bottomAnchor constraintEqualToAnchor:overlayCard.bottomAnchor constant:-10.0],

        [headerButtons.heightAnchor constraintEqualToConstant:44.0],
        [self.startButton.heightAnchor constraintEqualToConstant:44.0],
        [self.pauseButton.heightAnchor constraintEqualToConstant:44.0],
        [self.settingButton.heightAnchor constraintEqualToConstant:44.0],
        [headerFaceSRItem.heightAnchor constraintEqualToConstant:44.0]
    ]];

    [self refreshSRControlMenus];
    [self updatePostResizeLabel];
    [self updatePauseButtonTitle];
    [self syncPreviewRenderState];
}

- (std::string)resourceRootPath {
    NSString *bundleRoot = [[NSBundle mainBundle] resourcePath];
    NSString *resource = [bundleRoot stringByAppendingPathComponent:@"resource"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:resource]) {
        return std::string(resource.UTF8String);
    }
    return std::string(bundleRoot.UTF8String);
}

- (NSString *)resourceRootNSString {
    std::string root = [self resourceRootPath];
    return [NSString stringWithUTF8String:root.c_str()];
}

- (void)setInfoText:(NSString *)text {
    NSString *safeText = text ?: @"";
    if (![self.infoLabel.text isEqualToString:safeText]) {
        //NSLog(@"[SRViewController] %@", safeText);
    }
    self.infoLabel.text = safeText;
}

- (void)bumpRenderRevision {
    self.renderRevision += 1;
}

- (void)syncPreviewRenderState {
    [self.glPreviewView updateRenderStateWithMode:self.srMode
                                              mix:self.mixSlider.value
                                            match:self.matchSlider.value
                                    displayScaleW:self.displayScaleWSlider.value
                                    displayScaleH:self.displayScaleHSlider.value
                                    faceSREnabled:self.faceSRSwitch.on
                                           paused:self.paused];
    [self.glPreviewView requestRender];
}

- (NSString *)perfPrefixWithProcessMs:(double)processMs {
    const double safeMs = processMs > 0.001 ? processMs : 0.001;
    const double fps = 1000.0 / safeMs;
    return [NSString stringWithFormat:@"process: %.1fms fps: %.1f", processMs, fps];
}

- (void)initInferenceContextIfNeeded {
    if (self.inferenceContextReady) {
        return;
    }

    NSString *logDir = [NSTemporaryDirectory() stringByAppendingPathComponent:@"inference"];
    HJEntryContextInfo ctx;
    ctx.logIsValid = true;
    ctx.logDir = std::string(logDir.UTF8String);
    ctx.logLevel = HJLOGLevel_INFO;
    ctx.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
    ctx.logMaxFileSize = 5 * 1024 * 1024;
    ctx.logMaxFileNum = 5;

    int ret = inferenceContextInit(ctx);
    self.inferenceContextReady = (ret >= 0);
    if (!self.inferenceContextReady) {
        [self setInfoText:[NSString stringWithFormat:@"inferenceContextInit failed: %d", ret]];
    }
}

- (void)releaseInferenceContextIfNeeded {
    if (!self.inferenceContextReady) {
        return;
    }
    inferenceContextUnInit();
    self.inferenceContextReady = NO;
}

- (NSString *)imageSeqPath {
    NSString *root = [self resourceRootNSString];
    return [root stringByAppendingPathComponent:[NSString stringWithFormat:@"imgseq/%@", self.setting.inputFolder ?: kDefaultInputFolder]];
}

- (NSString *)imagePath {
    NSString *root = [self resourceRootNSString];
    return [root stringByAppendingPathComponent:[NSString stringWithFormat:@"image/%@", self.setting.imageFile ?: kDefaultImageName]];
}

- (BOOL)rebuildInputSource {
    _imageSeqWrapper.reset();
    self.singleImage = nil;

    if ([self.setting.inputSource isEqualToString:kInputSourceImage]) {
        UIImage *image = [UIImage imageWithContentsOfFile:[self imagePath]];
        self.singleImage = image;
        if (!image) {
            [self setInfoText:[NSString stringWithFormat:@"image load failed: %@", [self imagePath]]];
            return NO;
        }
        return YES;
    }

    auto wrapper = std::make_unique<HJImageSeqWrapper>();
    const std::string seqPath = std::string([self imageSeqPath].UTF8String);
    int ret = wrapper->init(seqPath, kSRFPS, HJConvertDataType_RGB, true);
    if (ret < 0) {
        [self setInfoText:[NSString stringWithFormat:@"seq init failed: %d (%@)", ret, [self imageSeqPath]]];
        return NO;
    }
    _imageSeqWrapper = std::move(wrapper);
    return YES;
}

- (int)currentSRType {
    if (self.setting.useVTProcessor) {
        return HJVideoSRWrapperType_VTFRAMEPROCESSOR;
    }
    if (self.setting.useCoreML) {
        return HJVideoSRWrapperType_COREMLREALESRGAN;
    }
    if ([self.setting.model isEqualToString:kModelRealCUGAN]) {
        return HJVideoSRWrapperType_NCNNREALCUGAN;
    }
    return HJVideoSRWrapperType_NCNNREALESRGAN;
}

- (BOOL)rebuildFaceSR {
    _faceSRWrapper.reset();
    auto wrapper = std::make_unique<HJFaceSRWrapper>();

    [self applyCoreMLSettingToModeAndPreScale];

    NSString *modelPath = [[self resourceRootNSString] stringByAppendingPathComponent:@"model"];
    const std::string modelDir = std::string(modelPath.UTF8String);
    HJFaceDetectWrapperOption faceOpt;
    faceOpt.ncnnRetinaFaceThreadNums = 2;//use 2, facedetect=1-2,sr=15-16; use 1 facedetect=5-7 sr=25ms why
    faceOpt.retinaFaceTargetSize = 200;
    faceOpt.ncnnRetinaFaceUseGPU = false;
    faceOpt.ncnnScrfdUseGPU = false;
    faceOpt.ncnnScrfdThreadNums = 1;
    faceOpt.coreMLRetinaFaceComputeMode = (int)self.setting.coreMLComputeMode;
    faceOpt.visionRectTargetSize = 160;
    faceOpt.visionRectComputeMode = 0;

    HJVideoSRWrapperOption srOpt;
    srOpt.ncnnUseGPU = self.setting.useGPU;
    srOpt.ncnnThreadNums = self.setting.useGPU ? 1 : (int)self.setting.threadNums;
    srOpt.ncnnScale = 2;
    if (self.setting.useCoreML) {
        self.setting.coreMLModelName = [self coreMLModelNameForCurrentSRMode];
    }
    srOpt.coreMLModelName = self.setting.coreMLModelName.length > 0 ? std::string(self.setting.coreMLModelName.UTF8String) : std::string();
    srOpt.coreMLComputeMode = (int)self.setting.coreMLComputeMode;
    if (self.setting.useVTProcessor) {
        srOpt.ncnnScale = 2;
    } else if (self.setting.useCoreML) {
        srOpt.ncnnScale = 4;
    } else if ([self.setting.model isEqualToString:kModelRealCUGAN]) {
        srOpt.ncnnRealCUGANType = std::string(self.setting.variant.UTF8String);
        srOpt.ncnnScale = 2;
    } else {
        srOpt.ncnnRealESRGANType = std::string(self.setting.variant.UTF8String);
        srOpt.ncnnScale = [self.setting.variant isEqualToString:@"realesr-general-x4v3"] ? 4 : 2;
    }

    CGSize preScale = [self resolvedPreScaleSize];
    NSLog(@"[SRViewController] rebuildFaceSR detectBackend=%@ retinaTarget=%d retinaCompute=%d coreml=%d scaleMode=%ld currentModel=%@ faceModel=%@ fullModel=%@ srMode=%ld pre=%dx%d",
          @"COREMLRETINAFACE",
          faceOpt.retinaFaceTargetSize,
          faceOpt.coreMLRetinaFaceComputeMode,
          self.setting.useCoreML,
          (long)self.setting.coreMLScaleMode,
          self.setting.coreMLModelName ?: @"",
          self.setting.coreMLFaceScaleModelName ?: @"",
          self.setting.coreMLFullScaleModelName ?: @"",
          (long)self.srMode,
          (int)preScale.width,
          (int)preScale.height);

    int ret = wrapper->init(modelDir, HJFaceDetectWrapperType_COREMLRETINAFACE, (HJVideoSRWrapperType)[self currentSRType], faceOpt, srOpt);
    if (ret < 0) {
        [self setInfoText:[NSString stringWithFormat:@"initFaceSR failed: %d", ret]];
        return NO;
    }
    _faceSRWrapper = std::move(wrapper);
    return YES;
}

- (NSArray<NSString *> *)coreMLModelOptionsForFolder:(NSString *)folder {
    NSFileManager *fm = [NSFileManager defaultManager];
    NSString *coreMLDir = [[self resourceRootNSString] stringByAppendingPathComponent:folder];
    NSArray<NSString *> *entries = [fm contentsOfDirectoryAtPath:coreMLDir error:nil] ?: @[];
    NSMutableOrderedSet<NSString *> *modelSet = [NSMutableOrderedSet orderedSet];
    for (NSString *name in entries) {
        NSString *ext = name.pathExtension.lowercaseString;
        if (!([ext isEqualToString:@"mlpackage"] || [ext isEqualToString:@"mlmodel"] || [ext isEqualToString:@"mlmodelc"])) {
            continue;
        }
        [modelSet addObject:name.stringByDeletingPathExtension];
    }
    NSMutableArray<NSString *> *models = [modelSet.array mutableCopy];
    [models sortUsingComparator:^NSComparisonResult(NSString *obj1, NSString *obj2) {
        BOOL obj1Fixed = [obj1 containsString:@"fixed-"];
        BOOL obj2Fixed = [obj2 containsString:@"fixed-"];
        if (obj1Fixed != obj2Fixed) {
            return obj1Fixed ? NSOrderedAscending : NSOrderedDescending;
        }
        return [obj1 compare:obj2 options:NSCaseInsensitiveSearch];
    }];
    return models;
}

- (NSArray<NSString *> *)coreMLModelOptions {
    NSMutableOrderedSet<NSString *> *modelSet = [NSMutableOrderedSet orderedSet];
    for (NSString *name in [self coreMLModelOptionsForFolder:kCoreMLFullScaleFolder]) {
        [modelSet addObject:name];
    }
    for (NSString *name in [self coreMLModelOptionsForFolder:kCoreMLFaceScaleFolder]) {
        [modelSet addObject:name];
    }
    NSMutableArray<NSString *> *models = [modelSet.array mutableCopy];
    [models sortUsingComparator:^NSComparisonResult(NSString *obj1, NSString *obj2) {
        BOOL obj1Fixed = [obj1 containsString:@"fixed-"];
        BOOL obj2Fixed = [obj2 containsString:@"fixed-"];
        if (obj1Fixed != obj2Fixed) {
            return obj1Fixed ? NSOrderedAscending : NSOrderedDescending;
        }
        return [obj1 compare:obj2 options:NSCaseInsensitiveSearch];
    }];
    return models;
}

- (NSArray<NSString *> *)coreMLFullScaleModelOptions {
    return [self coreMLModelOptionsForFolder:kCoreMLFullScaleFolder];
}

- (NSArray<NSString *> *)coreMLFaceScaleModelOptions {
    return [self coreMLModelOptionsForFolder:kCoreMLFaceScaleFolder];
}

- (NSArray<NSValue *> *)scanFullScalePresetSizesFromCoreMLModels {
    NSMutableOrderedSet<NSString *> *sizeKeys = [NSMutableOrderedSet orderedSet];
    NSMutableArray<NSValue *> *sizes = [NSMutableArray array];
    for (NSString *name in [self coreMLFullScaleModelOptions]) {
        CGSize size = [self requiredPreScaleSizeForCoreMLModelName:name];
        if (CGSizeEqualToSize(size, CGSizeZero)) {
            continue;
        }
        NSString *key = [NSString stringWithFormat:@"%dx%d", (int)size.width, (int)size.height];
        if ([sizeKeys containsObject:key]) {
            continue;
        }
        [sizeKeys addObject:key];
        [sizes addObject:[NSValue valueWithCGSize:size]];
    }
    [sizes sortUsingComparator:^NSComparisonResult(NSValue *obj1, NSValue *obj2) {
        CGSize s1 = obj1.CGSizeValue;
        CGSize s2 = obj2.CGSizeValue;
        if ((int)s1.width != (int)s2.width) {
            return ((int)s1.width < (int)s2.width) ? NSOrderedAscending : NSOrderedDescending;
        }
        if ((int)s1.height != (int)s2.height) {
            return ((int)s1.height < (int)s2.height) ? NSOrderedAscending : NSOrderedDescending;
        }
        return NSOrderedSame;
    }];
    return sizes;
}

- (NSArray<NSValue *> *)scanFaceScalePresetSizesFromCoreMLModels {
    NSMutableOrderedSet<NSString *> *sizeKeys = [NSMutableOrderedSet orderedSet];
    NSMutableArray<NSValue *> *sizes = [NSMutableArray array];
    for (NSString *name in [self coreMLFaceScaleModelOptions]) {
        CGSize size = [self requiredPreScaleSizeForCoreMLModelName:name];
        if (CGSizeEqualToSize(size, CGSizeZero)) {
            continue;
        }
        NSString *key = [NSString stringWithFormat:@"%dx%d", (int)size.width, (int)size.height];
        if ([sizeKeys containsObject:key]) {
            continue;
        }
        [sizeKeys addObject:key];
        [sizes addObject:[NSValue valueWithCGSize:size]];
    }
    [sizes sortUsingComparator:^NSComparisonResult(NSValue *obj1, NSValue *obj2) {
        CGSize s1 = obj1.CGSizeValue;
        CGSize s2 = obj2.CGSizeValue;
        if ((int)s1.width != (int)s2.width) {
            return ((int)s1.width < (int)s2.width) ? NSOrderedAscending : NSOrderedDescending;
        }
        if ((int)s1.height != (int)s2.height) {
            return ((int)s1.height < (int)s2.height) ? NSOrderedAscending : NSOrderedDescending;
        }
        return NSOrderedSame;
    }];
    return sizes;
}

- (NSArray<NSString *> *)coreMLModelTitles {
    NSMutableArray<NSString *> *titles = [NSMutableArray array];
    for (NSString *name in [self coreMLModelOptions]) {
        NSString *title = name;
        if ([name containsString:@"fixed-"]) {
            CGSize size = [self requiredPreScaleSizeForCoreMLModelName:name];
            NSString *version = [name containsString:@"ios16"] ? @"v7" : @"v6";
            NSString *suffix = [name containsString:@"int8"] ? @"-int8" : @"";
            if ([name containsString:@"image"]) {
                suffix = [suffix stringByAppendingString:@"-image"];
            } else if ([name containsString:@"tensor"]) {
                suffix = [suffix stringByAppendingString:@"-tensor"];
            }
            if (!CGSizeEqualToSize(size, CGSizeZero)) {
                title = [NSString stringWithFormat:@"Fixed %@ %dx%d%@",
                         version,
                         (int)size.width,
                         (int)size.height,
                         suffix];
            }
        }
        [titles addObject:title];
    }
    return titles;
}

- (NSArray<NSString *> *)coreMLModelTitlesForOptions:(NSArray<NSString *> *)options {
    NSMutableArray<NSString *> *titles = [NSMutableArray array];
    for (NSString *name in options) {
        NSString *title = name;
        if ([name containsString:@"fixed-"]) {
            CGSize size = [self requiredPreScaleSizeForCoreMLModelName:name];
            NSString *version = [name containsString:@"ios16"] ? @"v7" : @"v6";
            NSString *suffix = [name containsString:@"int8"] ? @"-int8" : @"";
            if ([name containsString:@"image"]) {
                suffix = [suffix stringByAppendingString:@"-image"];
            } else if ([name containsString:@"tensor"]) {
                suffix = [suffix stringByAppendingString:@"-tensor"];
            }
            if (!CGSizeEqualToSize(size, CGSizeZero)) {
                title = [NSString stringWithFormat:@"Fixed %@ %dx%d%@",
                         version,
                         (int)size.width,
                         (int)size.height,
                         suffix];
            }
        }
        [titles addObject:title];
    }
    return titles;
}

- (void)startDisplayLink {
    [self.displayLink invalidate];
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayTick)];
    self.displayLink.preferredFramesPerSecond = kSRFPS;
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)stopDisplayLink {
    [self.displayLink invalidate];
    self.displayLink = nil;
}

- (void)startProcess {
    NSLog(@"[SRViewController] startProcess backend=%@ detectBackend=%@ retinaTarget=%d retinaCompute=%d useVT=%d useCoreML=%d input=%@/%@",
          self.setting.useVTProcessor ? @"VT" : (self.setting.useCoreML ? @"CoreML" : @"NCNN"),
          @"COREMLRETINAFACE",
          200,
          (int)self.setting.coreMLComputeMode,
          self.setting.useVTProcessor,
          self.setting.useCoreML,
          self.setting.inputSource ?: @"-",
          [self.setting.inputSource isEqualToString:kInputSourceImage] ? (self.setting.imageFile ?: @"-") : (self.setting.inputFolder ?: @"-"));
    [self initInferenceContextIfNeeded];
    if (!self.inferenceContextReady) {
        return;
    }
    if (![self rebuildInputSource]) {
        return;
    }
    if (![self rebuildFaceSR]) {
        return;
    }
    [self startDisplayLink];
    self.started = YES;
    self.paused = NO;
    [self.startButton setTitle:@"停止" forState:UIControlStateNormal];
    [self updatePauseButtonTitle];
    [self bumpRenderRevision];
    [self syncPreviewRenderState];
}

- (void)stopProcess {
    [self stopDisplayLink];
    // Drain in-flight processing blocks, but never block main thread for long.
    // VT initialization may wait on model download; waiting synchronously here can
    // stall iOS gesture handling and trigger "System gesture gate timed out".
    if (self.processQueue) {
        dispatch_group_t drainGroup = dispatch_group_create();
        dispatch_group_enter(drainGroup);
        dispatch_async(self.processQueue, ^{
            dispatch_group_leave(drainGroup);
        });
        long waitRet = dispatch_group_wait(drainGroup, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(200 * NSEC_PER_MSEC)));
        if (waitRet != 0) {
            NSLog(@"[SRViewController] stopProcess drain timed out; continue without blocking UI");
        }
    }
    self.started = NO;
    self.paused = NO;
    self.frameProcessing = NO;
    _imageSeqWrapper.reset();
    _faceSRWrapper.reset();
    [self.startButton setTitle:@"开始" forState:UIControlStateNormal];
    [self updatePauseButtonTitle];
    [self bumpRenderRevision];
    [self.glPreviewView clearAllFrames];
    [self syncPreviewRenderState];
}

- (void)restartProcess {
    BOOL wasRunning = self.started;
    [self stopProcess];
    if (wasRunning) {
        [self startProcess];
    }
}

- (void)onStartStopTapped {
    if (self.started) {
        [self stopProcess];
        return;
    }
    [self startProcess];
}

- (void)onPauseResumeTapped {
    if (!self.started) {
        return;
    }
    self.paused = !self.paused;
    [self updatePauseButtonTitle];
    [self bumpRenderRevision];
    [self syncPreviewRenderState];
}

- (void)onSettingTouchDown:(UIButton *)sender {
    (void)sender;
    NSLog(@"[SRViewController] setting button touch down");
}

- (void)onSettingTapped:(UIButton *)sender {
    (void)sender;
    NSLog(@"[SRViewController] setting button tapped");
    [self showSettingDialog];
}

- (std::shared_ptr<HJTransferMediaData>)transferFromUIImage:(UIImage *)image {
    if (!image || !image.CGImage) {
        return nullptr;
    }

    const int width = (int)CGImageGetWidth(image.CGImage);
    const int height = (int)CGImageGetHeight(image.CGImage);
    if (width <= 0 || height <= 0) {
        return nullptr;
    }

    const int stride = width * 4;
    const size_t byteSize = (size_t)stride * (size_t)height;
    if (byteSize > (size_t)INT_MAX) {
        return nullptr;
    }
    HJSPBuffer::Ptr buffer = HJSPBuffer::create((int)byteSize);
    if (!buffer || !buffer->getBuf()) {
        return nullptr;
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        buffer->getBuf(),
        width,
        height,
        8,
        stride,
        colorSpace,
        kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(colorSpace);
    if (!ctx) {
        return nullptr;
    }
    CGContextDrawImage(ctx, CGRectMake(0, 0, width, height), image.CGImage);
    CGContextRelease(ctx);

    unsigned char *planes[4] = { buffer->getBuf(), nullptr, nullptr, nullptr };
    int pitch[4] = { stride, 0, 0, 0 };
    auto frame = HJTransferMediaData::create(HJConvertDataType_RGBA, planes, pitch, width, height, CACurrentMediaTime() * 1000.0, HJConvertDataType_RGBA);
    return frame;
}

- (void)recoverFrameIfNeeded:(const std::shared_ptr<HJTransferMediaData> &)frame {
    if (!frame || [self.setting.inputSource isEqualToString:kInputSourceImage] || !_imageSeqWrapper) {
        return;
    }
    _imageSeqWrapper->recovery(frame);
}

- (std::shared_ptr<HJTransferMediaData>)nextInputFrame {
    if ([self.setting.inputSource isEqualToString:kInputSourceImage]) {
        return [self transferFromUIImage:self.singleImage];
    }
    if (!_imageSeqWrapper) {
        return nullptr;
    }
    return _imageSeqWrapper->acquire();
}

- (void)onDisplayTick {
    self.tickCounter += 1;
    if (!self.started || self.paused || self.frameProcessing || self.controlTracking) {
        return;
    }

    self.frameProcessing = YES;
    BOOL enableFaceSR = self.faceSRSwitch.on;
    BOOL enableNativePostSRDisplayResize = self.postResizeSwitch.on;
    NSUInteger frameRevision = self.renderRevision;
    CGSize previewDrawableSize = [self.glPreviewView drawablePixelSize];
    float displayScaleW = self.displayScaleWSlider.value;
    float displayScaleH = self.displayScaleHSlider.value;
    SRViewController *__weak weakSelf = self;
    dispatch_async(self.processQueue, ^{
        @autoreleasepool {
            CFTimeInterval processStart = CACurrentMediaTime();
            SRViewController *strongSelf = weakSelf;
            if (!strongSelf) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    SRViewController *uiSelf = weakSelf;
                    if (uiSelf) {
                        uiSelf.frameProcessing = NO;
                    }
                });
                return;
            }
            auto inputFrame = [strongSelf nextInputFrame];
            if (!inputFrame) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    SRViewController *uiSelf = weakSelf;
                    if (uiSelf) {
                        uiSelf.frameProcessing = NO;
                    }
                });
                return;
            }
            if (!enableFaceSR || !strongSelf->_faceSRWrapper) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    SRViewController *uiSelf = weakSelf;
                    if (!uiSelf) {
                        return;
                    }
                    [uiSelf recoverFrameIfNeeded:inputFrame];
                    if (uiSelf.renderRevision == frameRevision) {
                        [uiSelf.glPreviewView updateOriginFrame:inputFrame];
                        [uiSelf.glPreviewView clearSRFrame];
                        uiSelf.lastFaceTargetDisplayW = 0;
                        uiSelf.lastFaceTargetDisplayH = 0;
                        uiSelf.lastSROutputW = 0;
                        uiSelf.lastSROutputH = 0;
                        [uiSelf updatePostResizeLabel];
                        [uiSelf syncPreviewRenderState];
                    }
                    CFTimeInterval now = CACurrentMediaTime();
                    NSString *perf = [uiSelf perfPrefixWithProcessMs:(now - processStart) * 1000.0];
                    [uiSelf setInfoText:[NSString stringWithFormat:@"%@ FaceSR disabled", perf]];
                    uiSelf.frameProcessing = NO;
                });
                return;
            }
            HJFaceSRProcessResult processRet;
            CFTimeInterval srProcessStart = CACurrentMediaTime();
            HJFaceSRProcessOption processOption;
            if (strongSelf.setting.useCoreML) {
                [strongSelf applyCoreMLSettingToModeAndPreScale];
            }
            CGSize preScale = [strongSelf resolvedPreScaleSize];
            CGSize composeCanvasSize = HJComposeCanvasSize(previewDrawableSize,
                                                           displayScaleW,
                                                           displayScaleH,
                                                           inputFrame->getWidth(),
                                                           inputFrame->getHeight());
            processOption.mode = (HJFaceSRProcessMode)strongSelf.srMode;
            processOption.preScaleWidth = [strongSelf usesPreScaleMode:strongSelf.srMode] ? (int)preScale.width : 0;
            processOption.preScaleHeight = [strongSelf usesPreScaleMode:strongSelf.srMode] ? (int)preScale.height : 0;
            processOption.composeCanvasWidth = (int)lrint(composeCanvasSize.width);
            processOption.composeCanvasHeight = (int)lrint(composeCanvasSize.height);
            processOption.bEnablePostSRDisplayResize = enableNativePostSRDisplayResize;
            if (strongSelf.setting.useCoreML) {
                NSLog(@"[SRViewController] process coreml scaleMode=%ld currentModel=%@ faceModel=%@ fullModel=%@ mode=%ld pre=%dx%d canvas=(%d,%d) post=%@",
                      (long)strongSelf.setting.coreMLScaleMode,
                      strongSelf.setting.coreMLModelName ?: @"",
                      strongSelf.setting.coreMLFaceScaleModelName ?: @"",
                      strongSelf.setting.coreMLFullScaleModelName ?: @"",
                      (long)strongSelf.srMode,
                      processOption.preScaleWidth,
                      processOption.preScaleHeight,
                      processOption.composeCanvasWidth,
                      processOption.composeCanvasHeight,
                      enableNativePostSRDisplayResize ? @"nativeLanczos" : @"glLinear");
            }
            processOption.bFeather = [strongSelf isFaceMode:strongSelf.srMode];
            processOption.bMixedEnable = ([strongSelf currentSRType] != HJVideoSRWrapperType_COREMLREALESRGAN);
            processOption.mixAlphaRatio = strongSelf.mixSlider.value;
            processOption.policy = (HJFaceSRProcessPolicy)[strongSelf selectedProcPolicy];
            const int srRet = strongSelf->_faceSRWrapper->process(inputFrame, processOption, processRet);
            CFTimeInterval srProcessEnd = CACurrentMediaTime();
            const double processMs = (srProcessEnd - srProcessStart) * 1000.0;
            const double fdMs = (double)processRet.faceDetectElapsedMs;
            const double srMs = (double)processRet.srElapsedMs;
            const double otherMs = processMs - MAX(0.0, fdMs) - MAX(0.0, srMs);
            auto srOutput = strongSelf->_faceSRWrapper->takeLastOutput();
            const int srOutputWidth = srOutput ? srOutput->getWidth() : 0;
            const int srOutputHeight = srOutput ? srOutput->getHeight() : 0;
            if (srRet < 0 || !srOutput) {
                NSLog(@"[SRViewController] perf fail total=%.1f fd=%.1f sr=%.1f other=%.1f ret=%d mode=%ld pre=%dx%d output=%s srOutput=(%d,%d)",
                      processMs,
                      fdMs,
                      srMs,
                      otherMs,
                      srRet,
                      (long)strongSelf.srMode,
                      processOption.preScaleWidth,
                      processOption.preScaleHeight,
                      srOutput ? "yes" : "no",
                      srOutputWidth,
                      srOutputHeight);
                dispatch_async(dispatch_get_main_queue(), ^{
                    SRViewController *uiSelf = weakSelf;
                    if (!uiSelf) {
                        return;
                    }
                    [uiSelf recoverFrameIfNeeded:inputFrame];
                    if (uiSelf.renderRevision == frameRevision) {
                        [uiSelf.glPreviewView updateOriginFrame:inputFrame];
                        [uiSelf.glPreviewView clearSRFrame];
                        uiSelf.lastFaceTargetDisplayW = 0;
                        uiSelf.lastFaceTargetDisplayH = 0;
                        uiSelf.lastSROutputW = 0;
                        uiSelf.lastSROutputH = 0;
                        [uiSelf updatePostResizeLabel];
                        [uiSelf syncPreviewRenderState];
                    }
                    NSString *modeText = uiSelf.srModeOptions.count > uiSelf.srMode ? uiSelf.srModeOptions[uiSelf.srMode] : [NSString stringWithFormat:@"%ld", (long)uiSelf.srMode];
                    [uiSelf setInfoText:[NSString stringWithFormat:@"FaceSR failed ret:%d mode:%@ pre:%dx%d canvas:%dx%d post:%@ fd:%lld sr:%lld output:%@ srOut:(%d,%d)",
                                         srRet,
                                         modeText,
                                         processOption.preScaleWidth,
                                         processOption.preScaleHeight,
                                         processOption.composeCanvasWidth,
                                         processOption.composeCanvasHeight,
                                         [uiSelf postResizeModeText],
                                         processRet.faceDetectElapsedMs,
                                         processRet.srElapsedMs,
                                         srOutput ? @"yes" : @"no",
                                         srOutputWidth,
                                         srOutputHeight]];
                    uiSelf.frameProcessing = NO;
                });
                return;
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                SRViewController *uiSelf = weakSelf;
                if (!uiSelf) {
                    return;
                }
                if (uiSelf.renderRevision == frameRevision) {
                    [uiSelf.glPreviewView updateOriginFrame:inputFrame];
                    [uiSelf.glPreviewView updateSRFrame:srOutput result:processRet];
                    uiSelf.lastFaceTargetDisplayW = processRet.faceTargetDisplayWidth;
                    uiSelf.lastFaceTargetDisplayH = processRet.faceTargetDisplayHeight;
                    uiSelf.lastSROutputW = srOutputWidth;
                    uiSelf.lastSROutputH = srOutputHeight;
                    [uiSelf updatePostResizeLabel];
                    [uiSelf syncPreviewRenderState];
                }
                NSString *modeText = uiSelf.srModeOptions.count > uiSelf.srMode ? uiSelf.srModeOptions[uiSelf.srMode] : [NSString stringWithFormat:@"%ld", (long)uiSelf.srMode];
                NSLog(@"[SRViewController] perf ok total=%.1f fd=%.1f sr=%.1f other=%.1f mode=%@ pre=%dx%d rect=(%d,%d,%d,%d) scale=(%d,%d) canvas=(%d,%d) target=(%d,%d) srOutput=(%d,%d) preBlue=%lldms(%d,%d)->(%d,%d) postBlue=%lldms(%d,%d)->(%d,%d) post=%@",
                      processMs,
                      fdMs,
                      srMs,
                      otherMs,
                      modeText,
                      processOption.preScaleWidth,
                      processOption.preScaleHeight,
                      processRet.faceRect.x,
                      processRet.faceRect.y,
                      processRet.faceRect.w,
                      processRet.faceRect.h,
                      processRet.scaleW,
                      processRet.scaleH,
                      processOption.composeCanvasWidth,
                      processOption.composeCanvasHeight,
                      processRet.faceTargetDisplayWidth,
                      processRet.faceTargetDisplayHeight,
                      srOutputWidth,
                      srOutputHeight,
                      processRet.preScaleElapsedMs,
                      processRet.preScaleInputWidth,
                      processRet.preScaleInputHeight,
                      processRet.preScaleOutputWidth,
                      processRet.preScaleOutputHeight,
                      processRet.postResizeElapsedMs,
                      processRet.postResizeInputWidth,
                      processRet.postResizeInputHeight,
                      processRet.postResizeOutputWidth,
                      processRet.postResizeOutputHeight,
                      [uiSelf postResizeModeText]);
                NSString *perf = [uiSelf perfPrefixWithProcessMs:processMs];
                [uiSelf setInfoText:[NSString stringWithFormat:@"%@ ret:%d mode:%@ pre:%dx%d canvas:%dx%d post:%@ fd:%lldms sr:%lldms preBlue:%lldms (%d,%d)->(%d,%d) postBlue:%lldms (%d,%d)->(%d,%d) rect:(%d,%d,%d,%d)->scale:(%d,%d) target:(%d,%d) srOut:(%d,%d)",
                                     perf,
                                     srRet,
                                     modeText,
                                     processOption.preScaleWidth,
                                     processOption.preScaleHeight,
                                     processOption.composeCanvasWidth,
                                     processOption.composeCanvasHeight,
                                     [uiSelf postResizeModeText],
                                     processRet.faceDetectElapsedMs,
                                     processRet.srElapsedMs,
                                     processRet.preScaleElapsedMs,
                                     processRet.preScaleInputWidth,
                                     processRet.preScaleInputHeight,
                                     processRet.preScaleOutputWidth,
                                     processRet.preScaleOutputHeight,
                                     processRet.postResizeElapsedMs,
                                     processRet.postResizeInputWidth,
                                     processRet.postResizeInputHeight,
                                     processRet.postResizeOutputWidth,
                                     processRet.postResizeOutputHeight,
                                     processRet.faceRect.x,
                                     processRet.faceRect.y,
                                     processRet.faceRect.w,
                                     processRet.faceRect.h,
                                     processRet.scaleW,
                                     processRet.scaleH,
                                     processRet.faceTargetDisplayWidth,
                                     processRet.faceTargetDisplayHeight,
                                     srOutputWidth,
                                     srOutputHeight]];
                [uiSelf refreshFaceSRSwitchState];
                [uiSelf recoverFrameIfNeeded:inputFrame];
                uiSelf.frameProcessing = NO;
            });
        }
    });
}

- (void)refreshFaceSRSwitchState {
    [self updateFaceSRStatusLabel];
    [self updatePostResizeLabel];
    [self updateSettingLabel];
    [self syncPreviewRenderState];
}

- (NSString *)coreMLComputeModeText:(NSInteger)mode {
    switch ((HJCoreMLComputeMode)mode) {
        case HJCoreMLComputeModeAll: return @"All";
        case HJCoreMLComputeModeCPUAndGPU: return @"CPU+GPU";
        case HJCoreMLComputeModeCPUAndNeuralEngine: return @"CPU+ANE";
        case HJCoreMLComputeModeCPUOnly: return @"CPU";
        default: return @"CPU+ANE";
    }
}

- (void)updateSettingLabel {
    NSString *inputDesc = [self.setting.inputSource isEqualToString:kInputSourceImage]
        ? [NSString stringWithFormat:@"image:%@", self.setting.imageFile ?: @""]
        : [NSString stringWithFormat:@"imgseq:%@", self.setting.inputFolder ?: @""];
    NSString *backend = self.setting.useVTProcessor ? @"VT" : (self.setting.useCoreML ? @"CoreML" : @"NCNN");
    NSString *deviceDesc = self.setting.useVTProcessor
        ? @"AppleVT"
        : (self.setting.useCoreML ? [self coreMLComputeModeText:self.setting.coreMLComputeMode] : (self.setting.useGPU ? @"GPU" : @"CPU"));
    NSString *modelDesc = self.setting.useVTProcessor
        ? @"superres-x2"
        : (self.setting.useCoreML ? (self.setting.coreMLModelName ?: @"coreml-default") : (self.setting.model ?: @"-"));
    NSString *variantDesc = self.setting.useVTProcessor
        ? @"sequential"
        : (self.setting.useCoreML ? @"mlmodelc/mlpackage" : (self.setting.variant ?: @"-"));
    NSString *postDesc = [self postResizeModeText];
    self.settingLabel.text = [NSString stringWithFormat:@"SR[%@]: %@ / %@ / %@ / t%ld / %@ / post:%@",
                              backend,
                              deviceDesc,
                              modelDesc,
                              variantDesc,
                              (long)self.setting.threadNums,
                              inputDesc,
                              postDesc];
}

- (void)onFaceSRToggled:(UISwitch *)sender {
    (void)sender;
    self.controlTracking = NO;
    [self bumpRenderRevision];
    [self refreshFaceSRSwitchState];
}

- (void)onPostResizeToggled:(UISwitch *)sender {
    (void)sender;
    self.controlTracking = NO;
    [self updatePostResizeLabel];
    [self updateSettingLabel];
    [self bumpRenderRevision];
    [self syncPreviewRenderState];
}

- (void)onMixChanged:(UISlider *)sender {
    (void)sender;
    [self updateMixLabel];
    [self bumpRenderRevision];
    [self syncPreviewRenderState];
}

- (void)onMatchChanged:(UISlider *)sender {
    (void)sender;
    [self updateMatchLabel];
    [self bumpRenderRevision];
    [self syncPreviewRenderState];
}

- (void)onDisplayScaleWChanged:(UISlider *)sender {
    self.displayScaleW = sender.value;
    [self updateDisplayScaleLabels];
    [self bumpRenderRevision];
    [self syncPreviewRenderState];
}

- (void)onDisplayScaleHChanged:(UISlider *)sender {
    self.displayScaleH = sender.value;
    [self updateDisplayScaleLabels];
    [self bumpRenderRevision];
    [self syncPreviewRenderState];
}

- (void)onControlTouchDown:(UIControl *)sender {
    (void)sender;
    self.controlTracking = YES;
}

- (void)onControlTouchUp:(UIControl *)sender {
    (void)sender;
    self.controlTracking = NO;
    [self syncPreviewRenderState];
}

- (NSArray<NSString *> *)scanImageFiles {
    NSString *imageDir = [[[self resourceRootNSString] stringByAppendingPathComponent:@"image"] copy];
    NSArray<NSString *> *entries = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:imageDir error:nil] ?: @[];
    NSMutableArray<NSString *> *files = [NSMutableArray array];
    for (NSString *name in entries) {
        NSString *lower = name.lowercaseString;
        if ([lower hasSuffix:@".jpg"] || [lower hasSuffix:@".jpeg"] || [lower hasSuffix:@".png"] || [lower hasSuffix:@".webp"] || [lower hasSuffix:@".bmp"]) {
            [files addObject:name];
        }
    }
    [files sortUsingSelector:@selector(compare:)];
    if (files.count == 0) {
        [files addObject:kDefaultImageName];
    }
    return files;
}

- (NSArray<NSString *> *)scanInputFolders {
    NSString *imgseqDir = [[[self resourceRootNSString] stringByAppendingPathComponent:@"imgseq"] copy];
    NSArray<NSString *> *entries = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:imgseqDir error:nil] ?: @[];
    NSMutableArray<NSString *> *folders = [NSMutableArray array];
    BOOL isDir = NO;
    for (NSString *name in entries) {
        NSString *fullPath = [imgseqDir stringByAppendingPathComponent:name];
        if ([[NSFileManager defaultManager] fileExistsAtPath:fullPath isDirectory:&isDir] && isDir) {
            [folders addObject:name];
        }
    }
    [folders sortUsingSelector:@selector(compare:)];
    if (folders.count == 0) {
        [folders addObject:kDefaultInputFolder];
    }
    return folders;
}

- (NSArray<NSString *> *)variantOptionsForModel:(NSString *)model {
    return [model isEqualToString:kModelRealCUGAN] ? self.cuganVariants : self.esrganVariants;
}

- (NSInteger)indexOfString:(NSString *)value inArray:(NSArray<NSString *> *)array defaultIndex:(NSInteger)defaultIndex {
    NSInteger idx = [array indexOfObject:value ?: @""];
    return idx == NSNotFound ? defaultIndex : idx;
}

- (NSInteger)indexOfThreadOption:(NSInteger)thread {
    for (NSInteger i = 0; i < (NSInteger)self.threadOptions.count; i++) {
        if (self.threadOptions[i].integerValue == thread) {
            return i;
        }
    }
    return 0;
}

- (NSInteger)indexOfSize:(CGSize)size inArray:(NSArray<NSValue *> *)array {
    for (NSInteger i = 0; i < (NSInteger)array.count; i++) {
        CGSize current = [array[i] CGSizeValue];
        if ((NSInteger)llround(current.width) == (NSInteger)llround(size.width) &&
            (NSInteger)llround(current.height) == (NSInteger)llround(size.height)) {
            return i;
        }
    }
    return NSNotFound;
}

- (CGSize)requiredPreScaleSizeForCoreMLModelName:(NSString *)modelName {
    if (modelName.length == 0 || [modelName rangeOfString:@"dynamic" options:NSCaseInsensitiveSearch].location != NSNotFound) {
        return CGSizeZero;
    }
    NSRange fixedRange = [modelName rangeOfString:@"fixed-"];
    if (fixedRange.location == NSNotFound) {
        return CGSizeZero;
    }
    NSString *suffix = [modelName substringFromIndex:(fixedRange.location + fixedRange.length)];
    NSInteger width = 0;
    NSInteger height = 0;
    if (sscanf(suffix.UTF8String, "%ldx%ld", &width, &height) == 2 && width > 0 && height > 0) {
        return CGSizeMake(width, height);
    }
    return CGSizeZero;
}

- (void)presentSettingMismatchAlert:(NSString *)message {
    UIViewController *presenter = self.settingDialogVC ?: self;
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"CoreML Model Mismatch"
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [presenter presentViewController:alert animated:YES completion:nil];
}

- (UIStackView *)makeSettingRowWithTitle:(NSString *)title control:(UIView *)control {
    UILabel *label = [[UILabel alloc] init];
    label.translatesAutoresizingMaskIntoConstraints = NO;
    label.text = title;
    label.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightSemibold];
    label.textColor = [UIColor labelColor];

    UIStackView *row = [[UIStackView alloc] initWithArrangedSubviews:@[ label, control ]];
    row.translatesAutoresizingMaskIntoConstraints = NO;
    row.axis = UILayoutConstraintAxisHorizontal;
    row.alignment = UIStackViewAlignmentCenter;
    row.distribution = UIStackViewDistributionEqualSpacing;
    row.spacing = 8.0;
    return row;
}

- (void)refreshSettingDialogDeviceState {
    NSInteger backendIdx = self.settingBackendSeg.selectedSegmentIndex;
    if (backendIdx == 1) {
        self.settingCoreMLUnitRow.hidden = NO;
        self.settingCoreMLUnitSeg.enabled = YES;
        self.settingCoreMLUnitSeg.alpha = 1.0;
        self.settingCoreMLScaleRow.hidden = NO;
        self.settingCoreMLScaleSeg.enabled = YES;
        self.settingCoreMLScaleSeg.alpha = 1.0;
        self.settingCoreMLModelRow.hidden = NO;
        self.settingCoreMLModelButton.enabled = YES;
        self.settingCoreMLModelButton.alpha = 1.0;
        self.settingDeviceSeg.enabled = NO;
        self.settingDeviceSeg.alpha = 0.5;
        self.settingModelSeg.enabled = NO;
        self.settingModelSeg.alpha = 0.5;
        self.settingVariantSeg.enabled = NO;
        self.settingVariantSeg.alpha = 0.5;
        self.settingThreadSeg.enabled = NO;
        self.settingThreadSeg.alpha = 0.5;
        return;
    }
    if (backendIdx == 2) {
        self.settingCoreMLUnitRow.hidden = YES;
        self.settingCoreMLUnitSeg.enabled = NO;
        self.settingCoreMLUnitSeg.alpha = 0.5;
        self.settingCoreMLScaleRow.hidden = YES;
        self.settingCoreMLScaleSeg.enabled = NO;
        self.settingCoreMLScaleSeg.alpha = 0.5;
        self.settingCoreMLModelRow.hidden = YES;
        self.settingCoreMLModelButton.enabled = NO;
        self.settingCoreMLModelButton.alpha = 0.5;
        self.settingDeviceSeg.enabled = NO;
        self.settingDeviceSeg.alpha = 0.5;
        self.settingModelSeg.enabled = NO;
        self.settingModelSeg.alpha = 0.5;
        self.settingVariantSeg.enabled = NO;
        self.settingVariantSeg.alpha = 0.5;
        self.settingThreadSeg.enabled = NO;
        self.settingThreadSeg.alpha = 0.5;
        return;
    }
    self.settingCoreMLUnitRow.hidden = YES;
    self.settingCoreMLUnitSeg.enabled = NO;
    self.settingCoreMLUnitSeg.alpha = 0.5;
    self.settingCoreMLScaleRow.hidden = YES;
    self.settingCoreMLScaleSeg.enabled = NO;
    self.settingCoreMLScaleSeg.alpha = 0.5;
    self.settingCoreMLModelRow.hidden = YES;
    self.settingCoreMLModelButton.enabled = NO;
    self.settingCoreMLModelButton.alpha = 0.5;
    self.settingDeviceSeg.enabled = YES;
    self.settingDeviceSeg.alpha = 1.0;
    self.settingModelSeg.enabled = YES;
    self.settingModelSeg.alpha = 1.0;
    self.settingVariantSeg.enabled = YES;
    self.settingVariantSeg.alpha = 1.0;

    BOOL useGPU = (self.settingDeviceSeg.selectedSegmentIndex == 1);
    if (useGPU) {
        self.settingThreadSeg.selectedSegmentIndex = [self indexOfThreadOption:1];
        self.settingThreadSeg.enabled = NO;
        self.settingThreadSeg.alpha = 0.5;
    } else {
        NSInteger idx = self.settingThreadSeg.selectedSegmentIndex;
        if (idx < 0 || idx >= (NSInteger)self.threadOptions.count) {
            idx = [self indexOfThreadOption:2];
        }
        if (self.threadOptions[idx].integerValue == 1) {
            idx = [self indexOfThreadOption:2];
        }
        self.settingThreadSeg.selectedSegmentIndex = idx;
        self.settingThreadSeg.enabled = YES;
        self.settingThreadSeg.alpha = 1.0;
    }
}

- (void)onSettingBackendChanged:(UISegmentedControl *)sender {
    (void)sender;
    [self refreshSettingDialogDeviceState];
}

- (void)onSettingCoreMLScaleChanged:(UISegmentedControl *)sender {
    NSInteger scaleMode = MAX(0, sender.selectedSegmentIndex);
    NSArray<NSString *> *options = [self coreMLModelOptionsForScaleMode:scaleMode];
    NSArray<NSString *> *titles = [self coreMLModelTitlesForOptions:options];
    NSInteger idx = [self indexOfString:[self coreMLModelNameFromSettingForScaleMode:scaleMode]
                                 inArray:options
                            defaultIndex:0];
    [self configureSettingMenuButton:self.settingCoreMLModelButton
                              titles:titles
                       selectedIndex:idx
                             handler:nil];
}

- (void)refreshSettingDialogModelState {
    NSString *model = self.settingModelSeg.selectedSegmentIndex == 1 ? kModelRealCUGAN : kModelRealESRGAN;
    NSArray<NSString *> *items = [self variantOptionsForModel:model];
    [self.settingVariantSeg removeAllSegments];
    for (NSUInteger i = 0; i < items.count; i++) {
        [self.settingVariantSeg insertSegmentWithTitle:items[i] atIndex:i animated:NO];
    }
    NSInteger idx = [self indexOfString:self.setting.variant inArray:items defaultIndex:0];
    self.settingVariantSeg.selectedSegmentIndex = idx;
}

- (void)refreshSettingDialogInputSourceState {
    BOOL imageMode = (self.settingSourceButton.tag == 1);
    self.settingFolderRow.hidden = imageMode;
    self.settingImageRow.hidden = !imageMode;
}

- (void)onSettingDeviceChanged:(UISegmentedControl *)sender {
    (void)sender;
    [self refreshSettingDialogDeviceState];
}

- (void)onSettingModelChanged:(UISegmentedControl *)sender {
    (void)sender;
    [self refreshSettingDialogModelState];
}

- (void)onSettingInputSourceChanged {
    [self refreshSettingDialogInputSourceState];
}

- (void)onSettingDialogCancelTapped:(UIButton *)sender {
    (void)sender;
    [self.settingDialogVC dismissViewControllerAnimated:YES completion:nil];
}

- (void)onSettingDialogApplyTapped:(UIButton *)sender {
    (void)sender;
    NSInteger backendIdx = self.settingBackendSeg.selectedSegmentIndex;
    self.setting.useCoreML = (backendIdx == 1);
    self.setting.useVTProcessor = (backendIdx == 2);
    NSInteger cmIdx = MAX(0, self.settingCoreMLUnitSeg.selectedSegmentIndex);
    if (cmIdx > 3) {
        cmIdx = HJCoreMLComputeModeAll;
    }
    self.setting.coreMLComputeMode = cmIdx;
    self.setting.coreMLScaleMode = MAX(0, self.settingCoreMLScaleSeg.selectedSegmentIndex);
    NSArray<NSString *> *coreMLModels = [self selectedCoreMLModelOptions];
    NSInteger modelIdx = MAX(0, self.settingCoreMLModelButton.tag);
    if (modelIdx >= (NSInteger)coreMLModels.count) {
        modelIdx = (NSInteger)coreMLModels.count - 1;
    }
    if (modelIdx < 0) {
        modelIdx = 0;
    }
    NSString *selectedCoreMLModelName = coreMLModels.count > 0 ? coreMLModels[modelIdx] : @"";
    if (self.setting.coreMLScaleMode == HJCoreMLScaleModeFaceScale) {
        self.setting.coreMLFaceScaleModelName = selectedCoreMLModelName;
    } else {
        self.setting.coreMLFullScaleModelName = selectedCoreMLModelName;
    }
    self.setting.coreMLModelName = selectedCoreMLModelName;
    self.setting.useGPU = (self.settingDeviceSeg.selectedSegmentIndex == 1);
    self.setting.model = (self.settingModelSeg.selectedSegmentIndex == 1) ? kModelRealCUGAN : kModelRealESRGAN;

    NSArray<NSString *> *variantOptions = [self variantOptionsForModel:self.setting.model];
    NSInteger variantIdx = MAX(0, self.settingVariantSeg.selectedSegmentIndex);
    if (variantIdx >= (NSInteger)variantOptions.count) {
        variantIdx = 0;
    }
    self.setting.variant = variantOptions[variantIdx];

    if (self.setting.useGPU) {
        self.setting.threadNums = 1;
    } else {
        NSInteger threadIdx = MAX(0, self.settingThreadSeg.selectedSegmentIndex);
        if (threadIdx >= (NSInteger)self.threadOptions.count) {
            threadIdx = [self indexOfThreadOption:2];
        }
        self.setting.threadNums = self.threadOptions[threadIdx].integerValue;
        if (self.setting.threadNums <= 0) {
            self.setting.threadNums = 2;
        }
    }

    self.setting.inputSource = (self.settingSourceButton.tag == 1) ? kInputSourceImage : kInputSourceImageSeq;

    NSInteger folderIdx = MAX(0, self.settingFolderButton.tag);
    if (folderIdx >= (NSInteger)self.inputFolderOptions.count) {
        folderIdx = 0;
    }
    self.setting.inputFolder = self.inputFolderOptions[folderIdx];

    NSInteger imageIdx = MAX(0, self.settingImageButton.tag);
    if (imageIdx >= (NSInteger)self.imageFileOptions.count) {
        imageIdx = 0;
    }
    self.setting.imageFile = self.imageFileOptions[imageIdx];

    if (self.setting.useCoreML) {
        CGSize requiredSize = [self requiredPreScaleSizeForCoreMLModelName:selectedCoreMLModelName];
        if (CGSizeEqualToSize(requiredSize, CGSizeZero)) {
            NSString *message = [NSString stringWithFormat:@"CoreML model %@ is not a fixed-size model.", selectedCoreMLModelName];
            [self presentSettingMismatchAlert:message];
            return;
        }
        if (self.setting.coreMLScaleMode == HJCoreMLScaleModeFaceScale) {
            NSInteger faceScaleIdx = [self indexOfSize:requiredSize inArray:self.faceScalePresetSizes];
            if (faceScaleIdx == NSNotFound) {
                NSString *message = [NSString stringWithFormat:@"CoreML fixed model %@ requires FaceScale PreScale %dx%d, but iOS presets do not contain this size.",
                                     selectedCoreMLModelName,
                                     (int)requiredSize.width,
                                     (int)requiredSize.height];
                [self presentSettingMismatchAlert:message];
                return;
            }
            self.srMode = HJFaceSRProcessMode_FaceScale;
            self.faceScalePresetIndex = faceScaleIdx;
        } else {
            NSInteger fullScaleIdx = [self indexOfSize:requiredSize inArray:self.fullScalePresetSizes];
            if (fullScaleIdx == NSNotFound) {
                NSString *message = [NSString stringWithFormat:@"CoreML fixed model %@ requires FullScale PreScale %dx%d, but iOS presets do not contain this size.",
                                     selectedCoreMLModelName,
                                     (int)requiredSize.width,
                                     (int)requiredSize.height];
                [self presentSettingMismatchAlert:message];
                return;
            }
            self.srMode = HJFaceSRProcessMode_FullScale;
            self.fullScalePresetIndex = fullScaleIdx;
        }
        [self applyCoreMLSettingToModeAndPreScale];
    }

    [self refreshSRControlMenus];
    [self updatePreScaleLabel];
    [self updateSettingLabel];
    [self restartProcess];
    [self.settingDialogVC dismissViewControllerAnimated:YES completion:nil];
}

- (void)showSettingDialog {
    self.inputFolderOptions = [self scanInputFolders];
    self.imageFileOptions = [self scanImageFiles];
    NSArray<NSString *> *variants = [self variantOptionsForModel:self.setting.model];

    UIViewController *dialogVC = [[UIViewController alloc] init];
    dialogVC.view.backgroundColor = [UIColor systemBackgroundColor];
    dialogVC.modalPresentationStyle = UIModalPresentationFormSheet;
    dialogVC.preferredContentSize = CGSizeMake(420.0, 520.0);

    UILabel *titleLabel = [[UILabel alloc] init];
    titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    titleLabel.text = @"SR Setting";
    titleLabel.font = [UIFont boldSystemFontOfSize:20.0];
    titleLabel.textColor = [UIColor labelColor];

    UISegmentedControl *deviceSeg = [[UISegmentedControl alloc] initWithItems:@[ @"CPU", @"GPU" ]];
    deviceSeg.translatesAutoresizingMaskIntoConstraints = NO;
    deviceSeg.selectedSegmentIndex = self.setting.useGPU ? 1 : 0;

    UISegmentedControl *backendSeg = [[UISegmentedControl alloc] initWithItems:@[ @"NCNN", @"CoreML", @"VT" ]];
    backendSeg.translatesAutoresizingMaskIntoConstraints = NO;
    backendSeg.selectedSegmentIndex = self.setting.useVTProcessor ? 2 : (self.setting.useCoreML ? 1 : 0);

    UISegmentedControl *coreMLUnitSeg = [[UISegmentedControl alloc] initWithItems:@[ @"All", @"GPU", @"ANE", @"CPU" ]];
    coreMLUnitSeg.translatesAutoresizingMaskIntoConstraints = NO;
    NSInteger coreMLUnitIdx = self.setting.coreMLComputeMode;
    if (coreMLUnitIdx < 0 || coreMLUnitIdx > 3) {
        coreMLUnitIdx = HJCoreMLComputeModeAll;
    }
    coreMLUnitSeg.selectedSegmentIndex = coreMLUnitIdx;

    UISegmentedControl *coreMLScaleSeg = [[UISegmentedControl alloc] initWithItems:@[ @"FullScale", @"FaceScale" ]];
    coreMLScaleSeg.translatesAutoresizingMaskIntoConstraints = NO;
    NSInteger coreMLScaleIdx = self.setting.coreMLScaleMode;
    if (coreMLScaleIdx < 0 || coreMLScaleIdx > 1) {
        coreMLScaleIdx = HJCoreMLScaleModeFullScale;
    }
    coreMLScaleSeg.selectedSegmentIndex = coreMLScaleIdx;

    UIButton *coreMLModelButton = [self makeSettingMenuButton];
    NSArray<NSString *> *initialCoreMLOptions = [self coreMLModelOptionsForScaleMode:coreMLScaleIdx];
    NSArray<NSString *> *coreMLTitles = [self coreMLModelTitlesForOptions:initialCoreMLOptions];
    NSInteger coreMLModelIdx = [self indexOfString:[self coreMLModelNameFromSettingForScaleMode:coreMLScaleIdx]
                                           inArray:initialCoreMLOptions
                                      defaultIndex:0];
    [self configureSettingMenuButton:coreMLModelButton
                              titles:coreMLTitles
                       selectedIndex:coreMLModelIdx
                             handler:nil];

    UISegmentedControl *modelSeg = [[UISegmentedControl alloc] initWithItems:@[ @"RealESRGAN", @"RealCUGAN" ]];
    modelSeg.translatesAutoresizingMaskIntoConstraints = NO;
    modelSeg.selectedSegmentIndex = [self.setting.model isEqualToString:kModelRealCUGAN] ? 1 : 0;

    UISegmentedControl *variantSeg = [[UISegmentedControl alloc] initWithItems:variants];
    variantSeg.translatesAutoresizingMaskIntoConstraints = NO;
    variantSeg.selectedSegmentIndex = [self indexOfString:self.setting.variant inArray:variants defaultIndex:0];

    NSMutableArray<NSString *> *threadItems = [NSMutableArray array];
    for (NSNumber *num in self.threadOptions) {
        [threadItems addObject:num.stringValue];
    }
    UISegmentedControl *threadSeg = [[UISegmentedControl alloc] initWithItems:threadItems];
    threadSeg.translatesAutoresizingMaskIntoConstraints = NO;
    threadSeg.selectedSegmentIndex = [self indexOfThreadOption:self.setting.threadNums];

    UIButton *sourceButton = [self makeSettingMenuButton];
    NSInteger sourceIdx = [self.setting.inputSource isEqualToString:kInputSourceImage] ? 1 : 0;
    [self configureSettingMenuButton:sourceButton
                              titles:@[ @"imageseq", @"image" ]
                       selectedIndex:sourceIdx
                             handler:^(__unused NSInteger idx) {
        [self onSettingInputSourceChanged];
    }];

    UIButton *folderButton = [self makeSettingMenuButton];
    NSInteger folderIdx = [self indexOfString:self.setting.inputFolder inArray:self.inputFolderOptions defaultIndex:0];
    [self configureSettingMenuButton:folderButton
                              titles:self.inputFolderOptions
                       selectedIndex:folderIdx
                             handler:nil];

    UIButton *imageButton = [self makeSettingMenuButton];
    NSInteger imageIdx = [self indexOfString:self.setting.imageFile inArray:self.imageFileOptions defaultIndex:0];
    [self configureSettingMenuButton:imageButton
                              titles:self.imageFileOptions
                       selectedIndex:imageIdx
                             handler:nil];

    UIStackView *folderRow = [self makeSettingRowWithTitle:@"Input Folder" control:folderButton];
    UIStackView *imageRow = [self makeSettingRowWithTitle:@"Image File" control:imageButton];
    UIStackView *coreMLUnitRow = [self makeSettingRowWithTitle:@"CoreML Compute" control:coreMLUnitSeg];
    UIStackView *coreMLScaleRow = [self makeSettingRowWithTitle:@"CoreML Scale" control:coreMLScaleSeg];
    UIStackView *coreMLModelRow = [self makeSettingRowWithTitle:@"CoreML Model" control:coreMLModelButton];

    UIStackView *form = [[UIStackView alloc] initWithArrangedSubviews:@[
        [self makeSettingRowWithTitle:@"Backend" control:backendSeg],
        coreMLUnitRow,
        coreMLScaleRow,
        coreMLModelRow,
        [self makeSettingRowWithTitle:@"Device" control:deviceSeg],
        [self makeSettingRowWithTitle:@"Model" control:modelSeg],
        [self makeSettingRowWithTitle:@"Variant" control:variantSeg],
        [self makeSettingRowWithTitle:@"Threads" control:threadSeg],
        [self makeSettingRowWithTitle:@"Input Source" control:sourceButton],
        folderRow,
        imageRow
    ]];
    form.translatesAutoresizingMaskIntoConstraints = NO;
    form.axis = UILayoutConstraintAxisVertical;
    form.spacing = 12.0;

    UIButton *cancelBtn = [UIButton buttonWithType:UIButtonTypeSystem];
    cancelBtn.translatesAutoresizingMaskIntoConstraints = NO;
    [cancelBtn setTitle:@"Cancel" forState:UIControlStateNormal];
    cancelBtn.titleLabel.font = [UIFont systemFontOfSize:16.0 weight:UIFontWeightSemibold];

    UIButton *applyBtn = [UIButton buttonWithType:UIButtonTypeSystem];
    applyBtn.translatesAutoresizingMaskIntoConstraints = NO;
    [applyBtn setTitle:@"Apply" forState:UIControlStateNormal];
    applyBtn.titleLabel.font = [UIFont systemFontOfSize:16.0 weight:UIFontWeightSemibold];

    UIStackView *buttonRow = [[UIStackView alloc] initWithArrangedSubviews:@[ cancelBtn, applyBtn ]];
    buttonRow.translatesAutoresizingMaskIntoConstraints = NO;
    buttonRow.axis = UILayoutConstraintAxisHorizontal;
    buttonRow.distribution = UIStackViewDistributionFillEqually;
    buttonRow.spacing = 12.0;

    UIView *content = dialogVC.view;
    [content addSubview:titleLabel];
    [content addSubview:form];
    [content addSubview:buttonRow];

    [NSLayoutConstraint activateConstraints:@[
        [titleLabel.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:20.0],
        [titleLabel.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-20.0],
        [titleLabel.topAnchor constraintEqualToAnchor:content.safeAreaLayoutGuide.topAnchor constant:16.0],

        [form.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:20.0],
        [form.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-20.0],
        [form.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor constant:16.0],

        [buttonRow.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:20.0],
        [buttonRow.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-20.0],
        [buttonRow.bottomAnchor constraintEqualToAnchor:content.safeAreaLayoutGuide.bottomAnchor constant:-16.0],
        [buttonRow.topAnchor constraintGreaterThanOrEqualToAnchor:form.bottomAnchor constant:16.0]
    ]];

    self.settingDialogVC = dialogVC;
    self.settingBackendSeg = backendSeg;
    self.settingCoreMLUnitSeg = coreMLUnitSeg;
    self.settingCoreMLScaleSeg = coreMLScaleSeg;
    self.settingCoreMLModelButton = coreMLModelButton;
    self.settingDeviceSeg = deviceSeg;
    self.settingModelSeg = modelSeg;
    self.settingVariantSeg = variantSeg;
    self.settingThreadSeg = threadSeg;
    self.settingSourceButton = sourceButton;
    self.settingFolderButton = folderButton;
    self.settingImageButton = imageButton;
    self.settingFolderRow = folderRow;
    self.settingImageRow = imageRow;
    self.settingCoreMLUnitRow = coreMLUnitRow;
    self.settingCoreMLScaleRow = coreMLScaleRow;
    self.settingCoreMLModelRow = coreMLModelRow;

    [deviceSeg addTarget:self action:@selector(onSettingDeviceChanged:) forControlEvents:UIControlEventValueChanged];
    [backendSeg addTarget:self action:@selector(onSettingBackendChanged:) forControlEvents:UIControlEventValueChanged];
    [coreMLScaleSeg addTarget:self action:@selector(onSettingCoreMLScaleChanged:) forControlEvents:UIControlEventValueChanged];
    [modelSeg addTarget:self action:@selector(onSettingModelChanged:) forControlEvents:UIControlEventValueChanged];
    [cancelBtn addTarget:self action:@selector(onSettingDialogCancelTapped:) forControlEvents:UIControlEventTouchUpInside];
    [applyBtn addTarget:self action:@selector(onSettingDialogApplyTapped:) forControlEvents:UIControlEventTouchUpInside];

    [self refreshSettingDialogDeviceState];
    [self refreshSettingDialogInputSourceState];

    [self presentViewController:dialogVC animated:YES completion:nil];
}

@end
