#import <UIKit/UIKit.h>

#ifdef __cplusplus
#include <memory>
namespace HJ {
class HJTransferMediaData;
}
struct HJFaceSRProcessResult;
#endif

NS_ASSUME_NONNULL_BEGIN

@interface HJSRGLView : UIView

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
- (void)requestRender;
- (CGSize)drawablePixelSize;

@end

NS_ASSUME_NONNULL_END
