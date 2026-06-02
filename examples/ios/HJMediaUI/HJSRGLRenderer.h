#import <Foundation/Foundation.h>
#import <OpenGLES/ES3/gl.h>
#import <UIKit/UIKit.h>

#ifdef __cplusplus
#include <memory>
namespace HJ {
class HJTransferMediaData;
}
struct HJFaceSRProcessResult;
#endif

NS_ASSUME_NONNULL_BEGIN

@interface HJSRGLRenderer : NSObject

- (BOOL)prepareIfNeeded;
- (void)invalidate;
- (void)setDrawableSize:(CGSize)drawableSize;
- (void)updateRenderStateWithMode:(NSInteger)mode
                              mix:(float)mix
                            match:(float)match
                    displayScaleW:(float)displayScaleW
                    displayScaleH:(float)displayScaleH
                    faceSREnabled:(BOOL)faceSREnabled
                           paused:(BOOL)paused;
#ifdef __cplusplus
- (void)updateOriginFrame:(const std::shared_ptr<HJ::HJTransferMediaData> &)frame;
- (void)updateSRFrame:(const std::shared_ptr<HJ::HJTransferMediaData> &)frame
               result:(const HJFaceSRProcessResult &)result;
#endif
- (void)clearSRFrame;
- (void)clearAllFrames;
- (BOOL)renderToFramebuffer:(GLuint)framebuffer;

@end

NS_ASSUME_NONNULL_END
