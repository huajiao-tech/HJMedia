#import "HJSRGLView.h"

#import <QuartzCore/CAEAGLLayer.h>
#import <QuartzCore/CADisplayLink.h>
#import <OpenGLES/ES3/gl.h>

#import "HJSRGLRenderer.h"

@interface HJSRGLView ()
@property (nonatomic, strong) EAGLContext *glContext;
@property (nonatomic, assign) GLuint framebuffer;
@property (nonatomic, assign) GLuint colorRenderbuffer;
@property (nonatomic, assign) GLint drawableWidth;
@property (nonatomic, assign) GLint drawableHeight;
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, strong) HJSRGLRenderer *renderer;
@property (nonatomic, assign) BOOL renderDirty;
@end

@implementation HJSRGLView

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self commonInit];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self) {
        [self commonInit];
    }
    return self;
}

- (void)dealloc
{
    [self.displayLink invalidate];
    self.displayLink = nil;
    if (self.glContext) {
        [EAGLContext setCurrentContext:self.glContext];
    }
    [self.renderer invalidate];
    [self tearDownBuffers];
    if ([EAGLContext currentContext] == self.glContext) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)commonInit
{
    self.contentScaleFactor = UIScreen.mainScreen.scale;
    self.opaque = YES;
    self.framebuffer = 0;
    self.colorRenderbuffer = 0;
    self.drawableWidth = 0;
    self.drawableHeight = 0;
    self.renderDirty = YES;
    [self configureLayer];
    [self setupContext];
    [self setupBuffers];
    self.renderer = [[HJSRGLRenderer alloc] init];
    [self startDisplayLink];
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    [self recreateDrawable];
    [self requestRender];
}

- (void)configureLayer
{
    CAEAGLLayer *layer = (CAEAGLLayer *)self.layer;
    layer.opaque = YES;
    layer.contentsScale = self.contentScaleFactor;
    layer.drawableProperties = @{
        kEAGLDrawablePropertyRetainedBacking : @(NO),
        kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
    };
}

- (void)setupContext
{
    self.glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!self.glContext) {
        self.glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    }
    [EAGLContext setCurrentContext:self.glContext];
}

- (void)setupBuffers
{
    if (!self.glContext) {
        return;
    }
    [EAGLContext setCurrentContext:self.glContext];
    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glGenRenderbuffers(1, &_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
    [self.glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRenderbuffer);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_drawableWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_drawableHeight);
    [self.renderer setDrawableSize:CGSizeMake(_drawableWidth, _drawableHeight)];
}

- (void)tearDownBuffers
{
    if (_framebuffer != 0) {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    if (_colorRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &_colorRenderbuffer);
        _colorRenderbuffer = 0;
    }
    _drawableWidth = 0;
    _drawableHeight = 0;
}

- (void)recreateDrawable
{
    if (!self.glContext) {
        return;
    }
    [EAGLContext setCurrentContext:self.glContext];
    [self tearDownBuffers];
    [self setupBuffers];
}

- (void)startDisplayLink
{
    [self.displayLink invalidate];
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayLinkTick)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)onDisplayLinkTick
{
    if (!self.renderDirty || self.framebuffer == 0 || self.colorRenderbuffer == 0) {
        return;
    }
    [self renderIfNeeded];
}

- (void)renderIfNeeded
{
    if (!self.glContext) {
        return;
    }
    [EAGLContext setCurrentContext:self.glContext];
    [self.renderer setDrawableSize:CGSizeMake(self.drawableWidth, self.drawableHeight)];
    glBindFramebuffer(GL_FRAMEBUFFER, self.framebuffer);
    if ([self.renderer renderToFramebuffer:self.framebuffer]) {
        glBindRenderbuffer(GL_RENDERBUFFER, self.colorRenderbuffer);
        [self.glContext presentRenderbuffer:GL_RENDERBUFFER];
    }
    self.renderDirty = NO;
}

- (void)updateRenderStateWithMode:(NSInteger)mode
                              mix:(float)mix
                            match:(float)match
                    displayScaleW:(float)displayScaleW
                    displayScaleH:(float)displayScaleH
                    faceSREnabled:(BOOL)faceSREnabled
                           paused:(BOOL)paused
{
    [EAGLContext setCurrentContext:self.glContext];
    [self.renderer updateRenderStateWithMode:mode
                                         mix:mix
                                       match:match
                               displayScaleW:displayScaleW
                               displayScaleH:displayScaleH
                               faceSREnabled:faceSREnabled
                                      paused:paused];
    [self requestRender];
}

- (void)updateOriginFrame:(const std::shared_ptr<HJ::HJTransferMediaData> &)frame
{
    [EAGLContext setCurrentContext:self.glContext];
    [self.renderer updateOriginFrame:frame];
    [self requestRender];
}

- (void)updateSRFrame:(const std::shared_ptr<HJ::HJTransferMediaData> &)frame
               result:(const HJFaceSRProcessResult &)result
{
    [EAGLContext setCurrentContext:self.glContext];
    [self.renderer updateSRFrame:frame result:result];
    [self requestRender];
}

- (void)clearSRFrame
{
    [EAGLContext setCurrentContext:self.glContext];
    [self.renderer clearSRFrame];
    [self requestRender];
}

- (void)clearAllFrames
{
    [EAGLContext setCurrentContext:self.glContext];
    [self.renderer clearAllFrames];
    [self requestRender];
}

- (void)requestRender
{
    self.renderDirty = YES;
}

- (CGSize)drawablePixelSize
{
    return CGSizeMake((CGFloat)self.drawableWidth, (CGFloat)self.drawableHeight);
}

@end
