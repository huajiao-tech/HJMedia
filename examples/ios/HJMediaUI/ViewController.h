#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, HJDemoMode) {
    HJDemoModeFaceu = 0,
    HJDemoModeSequenceOnly = 1,
};

@interface ViewController : UIViewController
- (instancetype)initWithMode:(HJDemoMode)mode
                       title:(NSString *)title
                sequencePath:(NSString *)sequencePath
               hideControls:(BOOL)hideControls;
@end
