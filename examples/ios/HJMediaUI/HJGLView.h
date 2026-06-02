#import <UIKit/UIKit.h>

@interface HJGLView : UIView
@property (nonatomic, assign) BOOL controlsHidden;
- (void)displayImageNamed:(NSString *)imageName;
- (void)displayImage:(UIImage *)image;
- (void)playImageSequenceAtRelativePath:(NSString *)relativePath;
- (void)stopImageSequence;
- (void)setResourcePath:(NSString*)resourcePath;
@end
