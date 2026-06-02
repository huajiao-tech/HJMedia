#import "HJSRGLRenderer.h"

#import <OpenGLES/ES3/glext.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "HJCommonInterface.h"
#include "HJOGCommon.h"
#include "HJOGFBOCtrl.h"
#include "HJTransferMediaData.h"
#include "utils/HJFaceSRWrapper.h"

NS_HJ_USING

namespace {

static const GLfloat kQuadVertices[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f,
};

static const GLfloat kTexCoordsDisplay[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
};

static const GLfloat kTexCoordsImage[] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
};

static GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }
    glDeleteShader(shader);
    return 0;
}

static GLuint buildProgram(const char *vertexSource, const char *fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return 0;
    }
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glBindAttribLocation(program, 0, "aPosition");
    glBindAttribLocation(program, 1, "aTexCoord");
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }
    glDeleteProgram(program);
    return 0;
}

static int clampToInt(int value, int lower, int upper)
{
    return std::max(lower, std::min(value, upper));
}

static CGSize clipOutputSize(CGSize targetSize, CGSize srcSize)
{
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
    return CGSizeMake(std::max(1.0, dw), std::max(1.0, dh));
}

}  // namespace

@interface HJSRGLRenderer ()
@end

@implementation HJSRGLRenderer
{
    GLuint _program;
    GLint _samplerLocation;
    GLint _mixLocation;

    GLuint _originTexture;
    int _originWidth;
    int _originHeight;

    GLuint _srTexture;
    int _srWidth;
    int _srHeight;

    std::shared_ptr<HJTransferMediaData> _originFrame;
    std::shared_ptr<HJTransferMediaData> _srFrame;
    HJFaceSRProcessResult _srResult;

    std::shared_ptr<HJOGFBOCtrl> _composeFbo;
    CGSize _drawableSize;
    NSInteger _mode;
    float _mix;
    float _match;
    float _displayScaleW;
    float _displayScaleH;
    BOOL _faceSREnabled;
    BOOL _paused;
    GLfloat _displayTexCoords[8];
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _program = 0;
        _samplerLocation = -1;
        _mixLocation = -1;
        _originTexture = 0;
        _srTexture = 0;
        _originWidth = 0;
        _originHeight = 0;
        _srWidth = 0;
        _srHeight = 0;
        _drawableSize = CGSizeZero;
        _mode = HJFaceSRProcessMode_FaceScale;
        _mix = 0.8f;
        _match = 1.0f;
        _displayScaleW = 1.0f;
        _displayScaleH = 1.0f;
        _faceSREnabled = YES;
        _paused = NO;
        _srResult = HJFaceSRProcessResult();
        memcpy(_displayTexCoords, kTexCoordsDisplay, sizeof(_displayTexCoords));
    }
    return self;
}

- (void)invalidate
{
    if (_program != 0) {
        glDeleteProgram(_program);
        _program = 0;
    }
    if (_originTexture != 0) {
        HJOGCommon::textureDestroy(_originTexture);
        _originTexture = 0;
    }
    if (_srTexture != 0) {
        HJOGCommon::textureDestroy(_srTexture);
        _srTexture = 0;
    }
    _composeFbo.reset();
    _originFrame.reset();
    _srFrame.reset();
    _originWidth = 0;
    _originHeight = 0;
    _srWidth = 0;
    _srHeight = 0;
}

- (BOOL)prepareIfNeeded
{
    if (_program != 0) {
        return YES;
    }
    const char *vertexSource =
        "#version 300 es\n"
        "layout(location = 0) in vec2 aPosition;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
        "  vTexCoord = aTexCoord;\n"
        "}\n";
    const char *fragmentSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec2 vTexCoord;\n"
        "uniform sampler2D uTexture;\n"
        "uniform float uMix;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  vec4 color = texture(uTexture, vTexCoord);\n"
        "  fragColor = vec4(color.rgb, color.a * uMix);\n"
        "}\n";
    _program = buildProgram(vertexSource, fragmentSource);
    if (_program == 0) {
        return NO;
    }
    _samplerLocation = glGetUniformLocation(_program, "uTexture");
    _mixLocation = glGetUniformLocation(_program, "uMix");
    return YES;
}

- (void)setDrawableSize:(CGSize)drawableSize
{
    _drawableSize = drawableSize;
}

- (void)updateRenderStateWithMode:(NSInteger)mode
                              mix:(float)mix
                            match:(float)match
                    displayScaleW:(float)displayScaleW
                    displayScaleH:(float)displayScaleH
                    faceSREnabled:(BOOL)faceSREnabled
                           paused:(BOOL)paused
{
    _mode = mode;
    _mix = std::max(0.0f, std::min(mix, 1.0f));
    _match = std::max(0.0f, std::min(match, 1.0f));
    _displayScaleW = std::max(0.0f, std::min(displayScaleW, 1.0f));
    _displayScaleH = std::max(0.0f, std::min(displayScaleH, 1.0f));
    _faceSREnabled = faceSREnabled;
    _paused = paused;
}

- (void)updateOriginFrame:(const std::shared_ptr<HJTransferMediaData> &)frame
{
    _originFrame = frame;
}

- (void)updateSRFrame:(const std::shared_ptr<HJTransferMediaData> &)frame
               result:(const HJFaceSRProcessResult &)result
{
    _srFrame = frame;
    _srResult = result;
}

- (void)clearSRFrame
{
    _srFrame.reset();
    _srResult = HJFaceSRProcessResult();
}

- (void)clearAllFrames
{
    [self clearSRFrame];
    _originFrame.reset();
}

- (std::shared_ptr<HJTransferMediaData>)rgbaFrameForFrame:(const std::shared_ptr<HJTransferMediaData> &)frame
{
    if (!frame) {
        return nullptr;
    }
    if (frame->getConvertType() == HJConvertDataType_RGBA) {
        return frame;
    }
    return HJTransferMediaData::create(frame, HJConvertDataType_RGBA);
}

- (BOOL)uploadFrame:(const std::shared_ptr<HJTransferMediaData> &)frame
          textureId:(GLuint *)textureId
              width:(int *)width
             height:(int *)height
{
    auto rgbaFrame = [self rgbaFrameForFrame:frame];
    if (!rgbaFrame || !rgbaFrame->getData(0)) {
        return NO;
    }
    const int frameWidth = rgbaFrame->getWidth();
    const int frameHeight = rgbaFrame->getHeight();
    if (frameWidth <= 0 || frameHeight <= 0) {
        return NO;
    }
    if (*textureId == 0) {
        *textureId = HJOGCommon::textureCreate();
    }
    if (*textureId == 0) {
        return NO;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, *textureId);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 frameWidth,
                 frameHeight,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 rgbaFrame->getData(0));
    glBindTexture(GL_TEXTURE_2D, 0);
    *width = frameWidth;
    *height = frameHeight;
    return YES;
}

- (BOOL)ensureComposeFboWithWidth:(int)width height:(int)height
{
    if (width <= 0 || height <= 0) {
        return NO;
    }
    if (_composeFbo && _composeFbo->getWidth() == width && _composeFbo->getHeight() == height) {
        return YES;
    }
    _composeFbo.reset();
    auto fbo = std::make_shared<HJOGFBOCtrl>();
    if (fbo->init(width, height, true) < 0) {
        return NO;
    }
    _composeFbo = fbo;
    return YES;
}

- (void)drawTexture:(GLuint)textureId texCoords:(const GLfloat *)texCoords mix:(float)mix
{
    glUseProgram(_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(_samplerLocation, 0);
    glUniform1f(_mixLocation, mix);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, kQuadVertices);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

- (CGRect)overlayRectForFboWidth:(int)fboWidth height:(int)fboHeight
{
    if (_mode == HJFaceSRProcessMode_Full || _mode == HJFaceSRProcessMode_FullScale) {
        return CGRectMake(0.0, 0.0, (CGFloat)fboWidth, (CGFloat)fboHeight);
    }
    if (_originWidth <= 0 || _originHeight <= 0 || _srResult.faceCount <= 0 || _srResult.faceRect.w <= 0 || _srResult.faceRect.h <= 0) {
        return CGRectZero;
    }
    const int originX = _srResult.faceRect.x;
    const int originY = _srResult.faceRect.y;
    const int originW = _srResult.faceRect.w;
    const int originH = _srResult.faceRect.h;
    int realX = (originX * fboWidth + (_originWidth / 2)) / _originWidth;
    int realY = ((_originHeight - originH - originY) * fboWidth + (_originWidth / 2)) / _originWidth;
    int realW = _srResult.faceTargetDisplayWidth;
    int realH = _srResult.faceTargetDisplayHeight;
    if (realW <= 0 || realH <= 0) {
        realW = (originW * fboWidth + (_originWidth / 2)) / _originWidth;
        realH = (originH * fboWidth + (_originWidth / 2)) / _originWidth;
    }
    realX = clampToInt(realX, 0, fboWidth);
    realY = clampToInt(realY, 0, fboHeight);
    realW = clampToInt(realW, 0, fboWidth - realX);
    realH = clampToInt(realH, 0, fboHeight - realY);
    if (realW <= 0 || realH <= 0) {
        return CGRectZero;
    }
    return CGRectMake((CGFloat)realX,
                      (CGFloat)realY,
                      (CGFloat)realW,
                      (CGFloat)realH);
}

- (void)drawSRToComposeFboWithRect:(CGRect)overlayRect
{
    if (_srTexture == 0 || CGRectIsEmpty(overlayRect) || _mix <= 0.001f) {
        return;
    }
    const GLint realX = (GLint)lrint(CGRectGetMinX(overlayRect));
    const GLint realY = (GLint)lrint(CGRectGetMinY(overlayRect));
    const GLint realW = (GLint)lrint(CGRectGetWidth(overlayRect));
    const GLint realH = (GLint)lrint(CGRectGetHeight(overlayRect));
    if (realW <= 0 || realH <= 0) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(realX, realY, realW, realH);

    const float overlayRatio = std::max(0.0f, std::min(_match, 1.0f));
    if (overlayRatio <= 0.0f) {
        glDisable(GL_BLEND);
        return;
    }

    const GLint overlayX = realX + (GLint)lrint((CGFloat)realW * (1.0f - overlayRatio));
    const GLint overlayW = std::max(0, realX + realW - overlayX);
    glEnable(GL_SCISSOR_TEST);
    glScissor(overlayX, realY, overlayW, realH);
    [self drawTexture:_srTexture texCoords:kTexCoordsImage mix:_mix];

    if (overlayRatio > 0.0f && overlayRatio < 1.0f) {
        glScissor(overlayX, realY, std::min(2, realW), realH);
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

- (void)updateDisplayTexCoordsForFboWidth:(int)fboWidth
                                  fboHeight:(int)fboHeight
                                targetWidth:(int)targetWidth
                               targetHeight:(int)targetHeight
{
    memcpy(_displayTexCoords, kTexCoordsDisplay, sizeof(_displayTexCoords));
    if (fboWidth <= 0 || fboHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
        return;
    }
    GLfloat minX = 0.0f;
    GLfloat maxX = 1.0f;
    GLfloat minY = 0.0f;
    GLfloat maxY = 1.0f;
    if (fboWidth > targetWidth) {
        const GLfloat visibleRatio = std::max(0.0f, std::min(1.0f, (GLfloat)targetWidth / (GLfloat)fboWidth));
        const GLfloat offset = (1.0f - visibleRatio) * 0.5f;
        minX = offset;
        maxX = 1.0f - offset;
    }
    if (fboHeight > targetHeight) {
        const GLfloat visibleRatio = std::max(0.0f, std::min(1.0f, (GLfloat)targetHeight / (GLfloat)fboHeight));
        const GLfloat offset = (1.0f - visibleRatio) * 0.5f;
        minY = offset;
        maxY = 1.0f - offset;
    }
    const GLfloat texCoords[] = {
        minX, minY,
        maxX, minY,
        minX, maxY,
        maxX, maxY,
    };
    memcpy(_displayTexCoords, texCoords, sizeof(_displayTexCoords));
}

- (BOOL)renderToFramebuffer:(GLuint)framebuffer
{
    if (![self prepareIfNeeded]) {
        return NO;
    }
    if (![self uploadFrame:_originFrame textureId:&_originTexture width:&_originWidth height:&_originHeight]) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return YES;
    }
    const int targetWidth = std::max(1, (int)lrint(_drawableSize.width));
    const int targetHeight = std::max(1, (int)lrint(_drawableSize.height));
    const int displayWidth = std::max(1, std::min(targetWidth, (int)lrint((double)targetWidth * _displayScaleW)));
    const int displayHeight = std::max(1, std::min(targetHeight, (int)lrint((double)targetHeight * _displayScaleH)));
    const int displayX = 0;
    const int displayY = std::max(0, targetHeight - displayHeight);
    const CGSize fboSize = clipOutputSize(CGSizeMake(displayWidth, displayHeight), CGSizeMake(_originWidth, _originHeight));
    const int fboWidth = std::max(1, (int)lrint(fboSize.width));
    const int fboHeight = std::max(1, (int)lrint(fboSize.height));
    const BOOL hasSR = _faceSREnabled && [self uploadFrame:_srFrame textureId:&_srTexture width:&_srWidth height:&_srHeight];
    if (![self ensureComposeFboWithWidth:fboWidth height:fboHeight]) {
        return NO;
    }

    _composeFbo->attach();
    glDisable(GL_BLEND);
    glViewport(0, 0, fboWidth, fboHeight);
    [self drawTexture:_originTexture texCoords:kTexCoordsImage mix:1.0f];

    if (hasSR) {
        CGRect overlayRect = [self overlayRectForFboWidth:fboWidth height:fboHeight];
        [self drawSRToComposeFboWithRect:overlayRect];
    }
    _composeFbo->detach();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    [self updateDisplayTexCoordsForFboWidth:fboWidth fboHeight:fboHeight targetWidth:displayWidth targetHeight:displayHeight];
    glViewport(displayX, displayY, displayWidth, displayHeight);
    [self drawTexture:_composeFbo->getTextureId() texCoords:_displayTexCoords mix:1.0f];
    return YES;
}

@end
