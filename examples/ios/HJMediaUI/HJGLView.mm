#import "HJGLView.h"

#import <QuartzCore/CAEAGLLayer.h>
#import <QuartzCore/CADisplayLink.h>
#import <CoreGraphics/CoreGraphics.h>
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#include "HJRenderFaceuExport.h"
#include "HJFacePointsMadeup.h"

#include <cstdint>
#include <memory>
#include <vector>

#import "HJGLImageRenderer.h"
#include "HJOGFBOCtrl.h"

@interface HJGLView () {
    std::unique_ptr<HJGLImageRenderer> _renderer;
    std::shared_ptr<HJFacePointsMadeup> m_facemadeup;
    std::shared_ptr<HJOGFBOCtrl> m_fbo;
    void *m_faceuhandle;
}
@property (nonatomic, strong) EAGLContext *glContext;
@property (nonatomic, assign) GLuint framebuffer;
@property (nonatomic, assign) GLuint colorRenderbuffer;
@property (nonatomic, assign) GLint drawableWidth;
@property (nonatomic, assign) GLint drawableHeight;
@property (nonatomic, strong) UIImage *pendingImage;
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, copy) NSArray<NSString *> *sequenceFramePaths;
@property (nonatomic, assign) CFTimeInterval sequenceLastFrameTimestamp;
@property (nonatomic, assign) double sequenceFrameDuration;
@property (nonatomic, copy) NSString *resourceRootOverride;
@property (nonatomic, strong) UIStackView *controlPanel;
@property (nonatomic, copy) NSString *faceuPath0;
@property (nonatomic, copy) NSString *faceuPath1;
@property (nonatomic, copy) NSString *faceuPath2;
@property (nonatomic, strong) NSMutableSet<NSString *> *activeFaceuKeys;
@property (nonatomic, assign) BOOL showBackgroundImage;
@end

@implementation HJGLView


static void FaceuNotificationCallback(const char* uniqueKey, int type) {
    NSString *identifier = uniqueKey ? [NSString stringWithUTF8String:uniqueKey] : @"";
    NSLog(@"Faceu notify %@ type %d", identifier, type);
}

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        [self commonInit];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    if (self = [super initWithCoder:coder]) {
        [self commonInit];
    }
    return self;
}

- (void)dealloc {
    [self stopImageSequence];
    if (self.glContext) {
        [EAGLContext setCurrentContext:self.glContext];
    }
    [self tearDownBuffers];
    if ([EAGLContext currentContext] == self.glContext) {
        [EAGLContext setCurrentContext:nil];
    }
    self.glContext = nil;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [self recreateDrawable];
}

- (void)commonInit {
    self.contentScaleFactor = UIScreen.mainScreen.scale;
    self.framebuffer = 0;
    self.colorRenderbuffer = 0;
    self.drawableWidth = 0;
    self.drawableHeight = 0;
    [self configureLayer];
    [self setupContext];
    [self setupBuffers];
    [self setupRenderer];
    
    m_faceuhandle = nullptr;
    m_facemadeup = nullptr;
    m_fbo = nullptr;
    self.activeFaceuKeys = [NSMutableSet set];
    [self refreshFaceuPaths];
    
    [self setupControlPanel];
}

- (void)configureLayer {
    CAEAGLLayer *layer = (CAEAGLLayer *)self.layer;
    layer.opaque = YES;
    layer.contentsScale = self.contentScaleFactor;
    layer.drawableProperties = @{ kEAGLDrawablePropertyRetainedBacking : @(NO),
                                  kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8 };
}

- (void)setupControlPanel {
    if (self.controlPanel) {
        return;
    }
    UIStackView *panel = [[UIStackView alloc] init];
    panel.axis = UILayoutConstraintAxisVertical;
    panel.spacing = 8.0;
    panel.alignment = UIStackViewAlignmentLeading;
    panel.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:panel];
    UILayoutGuide *guide = self.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [panel.leadingAnchor constraintEqualToAnchor:guide.leadingAnchor constant:12.0],
        [panel.topAnchor constraintEqualToAnchor:guide.topAnchor constant:12.0]
    ]];
    self.controlPanel = panel;
    self.controlPanel.hidden = self.controlsHidden;
    
    UIStackView *openRow = [[UIStackView alloc] init];
    openRow.axis = UILayoutConstraintAxisHorizontal;
    openRow.spacing = 6.0;
    openRow.alignment = UIStackViewAlignmentCenter;
    UILabel *openLabel = [[UILabel alloc] init];
    openLabel.text = @"Control";
    openLabel.textColor = [UIColor whiteColor];
    openLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold];
    [openLabel setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
    [openRow addArrangedSubview:openLabel];
    UIButton *openButton = [self buttonWithTitle:@"Open" tag:0 action:@selector(handleOpenButton:)];
    UIButton *closeButton = [self buttonWithTitle:@"Close" tag:0 action:@selector(handleCloseButton:)];
    UIButton *removeAllButton = [self buttonWithTitle:@"RemoveAll" tag:0 action:@selector(handleRemoveAllButton:)];
    UILabel *toggleLabel = [[UILabel alloc] init];
    toggleLabel.text = @"BG Image";
    toggleLabel.textColor = [UIColor whiteColor];
    toggleLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold];
    UISwitch *backgroundSwitch = [[UISwitch alloc] init];
    backgroundSwitch.on = self.showBackgroundImage;
    [backgroundSwitch addTarget:self action:@selector(handleBackgroundSwitch:) forControlEvents:UIControlEventValueChanged];
    [openRow addArrangedSubview:openButton];
    [openRow addArrangedSubview:closeButton];
    [openRow addArrangedSubview:removeAllButton];
    [openRow addArrangedSubview:toggleLabel];
    [openRow addArrangedSubview:backgroundSwitch];
    [self.controlPanel addArrangedSubview:openRow];
    
    for (NSInteger row = 0; row < 3; ++row) {
        UIStackView *rowStack = [[UIStackView alloc] init];
        rowStack.axis = UILayoutConstraintAxisHorizontal;
        rowStack.spacing = 6.0;
        rowStack.alignment = UIStackViewAlignmentCenter;
        UILabel *label = [[UILabel alloc] init];
        label.text = [NSString stringWithFormat:@"Row %ld", (long)(row + 1)];
        label.textColor = [UIColor whiteColor];
        label.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold];
        [label setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
        [rowStack addArrangedSubview:label];
        
        UIButton *addButton = [self buttonWithTitle:@"Add" tag:row action:@selector(handleAddButton:)];
        UIButton *removeButton = [self buttonWithTitle:@"Remove" tag:row action:@selector(handleRemoveButton:)];
        [rowStack addArrangedSubview:addButton];
        [rowStack addArrangedSubview:removeButton];
        [self.controlPanel addArrangedSubview:rowStack];
    }
}

- (UIButton *)buttonWithTitle:(NSString *)title tag:(NSInteger)tag action:(SEL)selector {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    button.tintColor = [UIColor whiteColor];
    button.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.45f];
    button.layer.cornerRadius = 4.0;
    button.tag = tag;
    button.titleLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold];
    [button addTarget:self action:selector forControlEvents:UIControlEventTouchUpInside];
    return button;
}

- (void)setControlsHidden:(BOOL)controlsHidden {
    _controlsHidden = controlsHidden;
    self.controlPanel.hidden = controlsHidden;
}

- (void)handleAddButton:(UIButton *)sender {
    
    if (!m_faceuhandle)
    {
        NSLog(@"hanle is null, not add");
        return;
    }
    
    NSLog(@"Add button tapped on row %ld", (long)sender.tag);
    NSString *faceuPath = [self faceuPathForRow:sender.tag];
    NSString *faceuKey = [self faceuKeyForRow:sender.tag];
    if (!faceuPath || !faceuKey || !m_faceuhandle) {
        NSLog(@"Faceu path/key missing for row %ld", (long)sender.tag);
        return;
    }
    if ([self.activeFaceuKeys containsObject:faceuKey]) {
        NSLog(@"Faceu %@ already added", faceuKey);
        return;
    }
    int err = faceuAdd(m_faceuhandle, [faceuKey UTF8String], [faceuPath UTF8String], false);
    if (err < 0) {
        NSLog(@"faceuAdd failed for %@ err:%d", faceuPath, err);
        return;
    }
    [self.activeFaceuKeys addObject:faceuKey];
}

- (void)handleRemoveButton:(UIButton *)sender {
    
    if (!m_faceuhandle)
    {
        NSLog(@"hanle is null, not add");
        return;
    }
    
    NSLog(@"Remove button tapped on row %ld", (long)sender.tag);
    if (!m_faceuhandle) {
        return;
    }
    NSString *faceuKey = [self faceuKeyForRow:sender.tag];
    if (!faceuKey.length) {
        return;
    }
    if (![self.activeFaceuKeys containsObject:faceuKey]) {
        NSLog(@"Faceu %@ not active", faceuKey);
        return;
    }
    int err = faceuRemove(m_faceuhandle, [faceuKey UTF8String]);
    if (err < 0) {
        NSLog(@"faceuRemove failed for key %@ err:%d", faceuKey, err);
        return;
    }
    [self.activeFaceuKeys removeObject:faceuKey];
}

- (void)handleOpenButton:(UIButton *)sender {
    NSLog(@"Open button tapped");
    [self selfaceuInit];
}

- (void)handleCloseButton:(UIButton *)sender {
    NSLog(@"Close button tapped");
    [self selfaceuUninit];
}

- (void)handleRemoveAllButton:(UIButton *)sender {
    NSLog(@"RemoveAll button tapped");
    if (m_faceuhandle) {
        faceuRemoveAll(m_faceuhandle);
    }
    [self.activeFaceuKeys removeAllObjects];
}

- (void)handleBackgroundSwitch:(UISwitch *)sender {
    self.showBackgroundImage = sender.isOn;
    NSLog(@"Background image toggle %@", self.showBackgroundImage ? @"ON" : @"OFF");
}

- (void)setupContext {
    self.glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!self.glContext) {
        NSLog(@"Failed to create OpenGL ES 3 context");
        return;
    }
    if (![EAGLContext setCurrentContext:self.glContext]) {
        NSLog(@"Failed to set current OpenGL context");
    }
}

- (BOOL)ensureContext {
    if (!self.glContext) {
        return NO;
    }
    if ([EAGLContext currentContext] != self.glContext) {
        if (![EAGLContext setCurrentContext:self.glContext]) {
            NSLog(@"Unable to make EAGLContext current");
            return NO;
        }
    }
    return YES;
}

- (void)setupBuffers {
    if (![self ensureContext]) {
        return;
    }
    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    
    glGenRenderbuffers(1, &_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
    [self.glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRenderbuffer);
    
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_drawableWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_drawableHeight);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"Framebuffer incomplete: 0x%04x", status);
    }
}

- (void)tearDownBuffers {
    if (![self ensureContext]) {
        return;
    }
    if (_framebuffer != 0) {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    if (_colorRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &_colorRenderbuffer);
        _colorRenderbuffer = 0;
    }
}

- (void)setupRenderer {
    if (!_renderer) {
        _renderer = std::make_unique<HJGLImageRenderer>();
    }
    if (![self ensureContext]) {
        NSLog(@"Cannot prepare renderer without active context");
        return;
    }
    if (!_renderer->isReady() && !_renderer->setup()) {
        NSLog(@"Failed to initialize OpenGL renderer");
        _renderer.reset();
        return;
    }
    _renderer->resize(static_cast<int>(self.drawableWidth), static_cast<int>(self.drawableHeight));
}

- (void)recreateDrawable {
    if (![self ensureContext]) {
        return;
    }
    [self tearDownBuffers];
    [self setupBuffers];
    if (_renderer) {
        _renderer->resize(static_cast<int>(self.drawableWidth), static_cast<int>(self.drawableHeight));
    }
    [self renderPendingImage];
}

- (void)displayImageNamed:(NSString *)imageName {
    if (imageName.length == 0) {
        return;
    }
    NSString *name = [imageName stringByDeletingPathExtension];
    NSString *ext = imageName.pathExtension;
    NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:ext.length == 0 ? @"jpg" : ext];
    UIImage *image = [UIImage imageWithContentsOfFile:path];
    if (!image) {
        NSLog(@"Failed to load image %@", imageName);
        return;
    }
    [self displayImage:image];
}

- (void)displayImage:(UIImage *)image {
    [self displayImage:image stopSequence:YES];
}

- (void)displayImage:(UIImage *)image stopSequence:(BOOL)stopSequence {
    if (!image) {
        return;
    }
    if (stopSequence) {
        [self stopImageSequence];
    }
    self.pendingImage = image;
    [self renderPendingImage];
}
- (void)setResourcePath:(NSString*)resourcePath
{
    if (resourcePath.length == 0) {
        return;
    }
    NSString *bundlePath = NSBundle.mainBundle.bundlePath;
    NSString *absolutePath = [bundlePath stringByAppendingPathComponent:resourcePath];
    self.resourceRootOverride = absolutePath;
    [self refreshFaceuPaths];
}
- (void)selfaceuInit
{
    if (m_faceuhandle)
    {
        NSLog(@"hanle is not null, not init");
        return;
    }
    
    m_faceuhandle = faceuInit(FaceuNotificationCallback, false);
    
    
    if (!m_facemadeup)
    {
        m_facemadeup = std::make_shared<HJFacePointsMadeup>();
    }
    
    if (!m_fbo)
    {
        m_fbo = std::make_shared<HJOGFBOCtrl>();
        m_fbo->init(720, 1280);
    }
}
- (void)selfaceuUninit
{
    if (!m_faceuhandle)
    {
        NSLog(@"hanle is null, not uninit");
        return;
    }
    
    faceuDone(m_faceuhandle);
    m_faceuhandle = nullptr;
    
    m_facemadeup = nullptr;
    m_fbo = nullptr;
}
- (void)playImageSequenceAtRelativePath:(NSString *)relativePath {
    if (relativePath.length == 0) {
        return;
    }
    NSString *base = [self baseResourcePath];
    NSString *sequenceDirectory = [base stringByAppendingPathComponent:relativePath];
    NSLog(@"Sequence directory %@ ", sequenceDirectory);
    BOOL isDirectory = NO;
    if (![[NSFileManager defaultManager] fileExistsAtPath:sequenceDirectory isDirectory:&isDirectory] || !isDirectory) {
        NSLog(@"Sequence directory %@ not found", sequenceDirectory);
        return;
    }
    
    NSDictionary *config = [self loadSequenceConfigAtDirectory:sequenceDirectory];
    NSString *prefix = [config objectForKey:@"prefix"];
    double fps = [[config objectForKey:@"fps"] doubleValue];
    if (fps <= 0.0) {
        fps = 30.0;
    }
    NSNumber *loopsNumber = [config objectForKey:@"loops"];
    NSUInteger loops = NSUIntegerMax;
    if (loopsNumber && loopsNumber.unsignedIntegerValue > 0) {
        loops = loopsNumber.unsignedIntegerValue;
    }
    
    NSArray<NSString *> *frames = [self framePathsAtDirectory:sequenceDirectory prefix:prefix];
    if (frames.count == 0) {
        NSLog(@"No image sequence frames found at %@", sequenceDirectory);
        return;
    }
    [self startImageSequencePlaybackWithFrames:frames fps:fps loops:loops];
}

- (void)stopImageSequence {
    if (self.displayLink) {
        [self.displayLink invalidate];
        self.displayLink = nil;
    }
    self.sequenceFramePaths = nil;
    self.sequenceLastFrameTimestamp = 0;
    self.sequenceFrameDuration = 0;
}

- (NSDictionary *)loadSequenceConfigAtDirectory:(NSString *)directory {
    NSString *configPath = [directory stringByAppendingPathComponent:@"config.json"];
    NSData *data = [NSData dataWithContentsOfFile:configPath];
    if (!data) {
        return @{};
    }
    NSError *error = nil;
    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
    if (error || ![json isKindOfClass:[NSDictionary class]]) {
        if (error) {
            NSLog(@"Failed to parse sequence config: %@", error);
        }
        return @{};
    }
    return json;
}

- (NSArray<NSString *> *)framePathsAtDirectory:(NSString *)directory prefix:(NSString *)prefix {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error = nil;
    NSArray<NSString *> *contents = [fileManager contentsOfDirectoryAtPath:directory error:&error];
    if (!contents) {
        NSLog(@"Unable to list directory %@: %@", directory, error);
        return @[];
    }
    
    NSCharacterSet *nonDigits = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
    NSSet<NSString *> *supportedExtensions = [NSSet setWithArray:@[@"jpg", @"jpeg", @"png", @"bmp"]];
    NSMutableArray<NSDictionary *> *frames = [NSMutableArray array];
    for (NSString *entry in contents) {
        NSString *fullPath = [directory stringByAppendingPathComponent:entry];
        BOOL isDirectory = NO;
        [fileManager fileExistsAtPath:fullPath isDirectory:&isDirectory];
        if (isDirectory) {
            continue;
        }
        NSString *extension = entry.pathExtension.lowercaseString;
        if (![supportedExtensions containsObject:extension]) {
            continue;
        }
        NSString *name = entry.stringByDeletingPathExtension;
        if (prefix.length > 0 && ![name hasPrefix:prefix]) {
            continue;
        }
        NSRange lastNonDigitRange = [name rangeOfCharacterFromSet:nonDigits options:NSBackwardsSearch];
        NSUInteger digitStart = (lastNonDigitRange.location == NSNotFound) ? 0 : lastNonDigitRange.location + 1;
        if (digitStart >= name.length) {
            continue;
        }
        NSString *digits = [name substringFromIndex:digitStart];
        if (digits.length == 0) {
            continue;
        }
        NSInteger frameIndex = digits.integerValue;
        [frames addObject:@{ @"path": fullPath, @"index": @(frameIndex) }];
    }
    
    [frames sortUsingComparator:^NSComparisonResult(NSDictionary *a, NSDictionary *b) {
        return [a[@"index"] compare:b[@"index"]];
    }];
    
    NSMutableArray<NSString *> *paths = [NSMutableArray arrayWithCapacity:frames.count];
    for (NSDictionary *entry in frames) {
        [paths addObject:entry[@"path"]];
    }
    return paths;
}

- (void)startImageSequencePlaybackWithFrames:(NSArray<NSString *> *)frames fps:(double)fps loops:(NSUInteger)loops {
    [self stopImageSequence];
    self.sequenceFramePaths = frames;
    self.sequenceFrameDuration = (1.0 / 30.0);
    self.sequenceLastFrameTimestamp = 0;
    [self presentSequenceFrameAtIndex:0];
    [self startDisplayLink];
}

- (void)startDisplayLink {
    if (self.displayLink) {
        [self.displayLink invalidate];
    }
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(handleDisplayLink:)];
    [self.displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}

- (void)renderFaceuFrame:(int &)outidx {
    if (!m_faceuhandle || !m_facemadeup) {
        return;
    }
    int outputIndex = 0;
    auto pointsReal = m_facemadeup->getFacePoints(-1, outputIndex);
    if (!pointsReal) {
        return;
    }
    outidx = outputIndex;
    
    const auto& filterPoints = pointsReal->getFilterPt();
    if (filterPoints.size() < 9) {
        return;
    }
//    HJPoint2D points[9];
//    for (size_t idx = 0; idx < 9; ++idx) {
//        points[idx].x = filterPoints[idx].x;
//        points[idx].y = filterPoints[idx].y;
//    }
    int width = m_facemadeup->getWidth();
    int height = m_facemadeup->getHeight();
    
    HJFacePointInfo morePoints;
    morePoints.width = width;
    morePoints.height = height;
    morePoints.faceCount = 1;
    if (morePoints.faceCount > 0)
    {
        morePoints.faces = new HJSingleFaceInfo[morePoints.faceCount];
        for (int i = 0; i < morePoints.faceCount; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                morePoints.faces[i].points[j].x = filterPoints[j].x;
                morePoints.faces[i].points[j].y = filterPoints[j].y;
            }
            morePoints.faces[i].rect.x = filterPoints[5].x;
            morePoints.faces[i].rect.y = filterPoints[5].y;
            morePoints.faces[i].rect.w = filterPoints[8].x - filterPoints[5].x;
            morePoints.faces[i].rect.h = filterPoints[8].y - filterPoints[5].y;
        }
    }
    unsigned char *output = nullptr;
    int err = faceuRender(m_faceuhandle, &morePoints, output);
    if (morePoints.faceCount > 0)
    {
        delete [] morePoints.faces;
    }
    if (err < 0) {
        NSLog(@"faceuRender failed with error %d", err);
        return;
    }
}
- (void)priDetailDraw
{
    if (_renderer == nullptr)
    {
        return;
    }
    
    int outIdx = 0;
    if (m_fbo)
    {
        m_fbo->attach();
        [self renderFaceuFrame:outIdx];
        m_fbo->detach();
    }

    
    glViewport(0, 0, self.drawableWidth, self.drawableHeight);
    
    if (self.showBackgroundImage && outIdx >= 0 && outIdx < self.sequenceFramePaths.count)
    {
        [self presentSequenceFrameAtIndex:outIdx];
    }
    if (m_fbo)
    {
        _renderer->draw(m_fbo->getTextureId());
    }
}
- (void)priRender
{
    if (![self ensureContext]) {
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, self.framebuffer);
    glViewport(0, 0, self.drawableWidth, self.drawableHeight);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    [self priDetailDraw];
    
    glBindRenderbuffer(GL_RENDERBUFFER, self.colorRenderbuffer);
    [self.glContext presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)handleDisplayLink:(CADisplayLink *)displayLink {
    if (self.sequenceFramePaths.count == 0 || self.sequenceFrameDuration <= 0.0) {
        return;
    }
    if (self.sequenceLastFrameTimestamp == 0) {
        self.sequenceLastFrameTimestamp = displayLink.timestamp;
        return;
    }
    CFTimeInterval elapsed = displayLink.timestamp - self.sequenceLastFrameTimestamp;
    if (elapsed < self.sequenceFrameDuration) {
        return;
    }
    self.sequenceLastFrameTimestamp = displayLink.timestamp;
    [self priRender];
}

- (void)presentSequenceFrameAtIndex:(NSUInteger)index {
    NSString *framePath = self.sequenceFramePaths[index];
    UIImage *frame = [UIImage imageWithContentsOfFile:framePath];
    if (!frame) {
        NSLog(@"Failed to load frame at path %@", framePath);
        return;
    }
    [self displayImage:frame stopSequence:NO];
}

static std::vector<uint8_t> CreatePixelBuffer(UIImage *image, size_t &width, size_t &height) {
    std::vector<uint8_t> pixels;
    CGImageRef cgImage = image.CGImage;
    if (!cgImage) {
        return pixels;
    }
    width = CGImageGetWidth(cgImage);
    height = CGImageGetHeight(cgImage);
    if (width == 0 || height == 0) {
        return pixels;
    }
    pixels.resize(width * height * 4);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(pixels.data(), width, height, 8, width * 4, colorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);
    if (!context) {
        pixels.clear();
        return pixels;
    }
    CGContextTranslateCTM(context, 0, height);
    CGContextScaleCTM(context, 1.0, -1.0);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
    CGContextRelease(context);
    return pixels;
}

- (void)renderPendingImage {
    if (!self.pendingImage || !_renderer) {
        return;
    }
    if (![self ensureContext]) {
        return;
    }
    size_t width = 0;
    size_t height = 0;
    std::vector<uint8_t> pixels = CreatePixelBuffer(self.pendingImage, width, height);
    if (pixels.empty()) {
        NSLog(@"Unable to convert image to pixel buffer");
        return;
    }
    _renderer->resize(static_cast<int>(self.drawableWidth), static_cast<int>(self.drawableHeight));
    if (!_renderer->uploadImage(pixels, static_cast<int>(width), static_cast<int>(height))) {
        NSLog(@"Uploading texture failed");
        return;
    }
    [self drawFrame];
    self.pendingImage = nil;
}

- (void)drawFrame {
    //    if (!_renderer || ![self ensureContext]) {
    //        return;
    //    }
    //    glBindFramebuffer(GL_FRAMEBUFFER, self.framebuffer);
    //    glViewport(0, 0, self.drawableWidth, self.drawableHeight);
    //    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    //    glClear(GL_COLOR_BUFFER_BIT);
    _renderer->draw();
    //    glBindRenderbuffer(GL_RENDERBUFFER, self.colorRenderbuffer);
    //    [self.glContext presentRenderbuffer:GL_RENDERBUFFER];
}


- (NSString *)baseResourcePath {
    if (self.resourceRootOverride.length > 0) {
        return self.resourceRootOverride;
    }
    NSString *bundlePath = NSBundle.mainBundle.bundlePath;
    NSString *resourcePath = [bundlePath stringByAppendingPathComponent:@"resource"];
    return resourcePath;
}

- (void)refreshFaceuPaths {
    NSString *base = [self.baseResourcePath stringByStandardizingPath];
    if (base.length == 0) {
        return;
    }
    self.faceuPath0 = [base stringByAppendingPathComponent:@"faceu/60031_10"];
    NSLog(@"m_faceupath0:  %@", self.faceuPath0);
    self.faceuPath1 = [base stringByAppendingPathComponent:@"faceu/90237_1"];
    self.faceuPath2 = [base stringByAppendingPathComponent:@"faceu/XianWeng"];
}

- (NSString *)faceuPathForRow:(NSInteger)row {
    switch (row) {
        case 0: return self.faceuPath0;
        case 1: return self.faceuPath1;
        case 2: return self.faceuPath2;
        default: return nil;
    }
}

- (NSString *)faceuKeyForRow:(NSInteger)row {
    switch (row) {
        case 0: return @"faceu_0";
        case 1: return @"faceu_1";
        case 2: return @"faceu_2";
        default: return nil;
    }
}
@end
