#import "ViewController.h"

#import "HJGLView.h"

@interface ViewController ()
@property (nonatomic, strong) HJGLView *glView;
@property (nonatomic, assign) HJDemoMode demoMode;
@property (nonatomic, copy) NSString *sequencePath;
@property (nonatomic, assign) BOOL hideControls;
@property (nonatomic, copy) NSString *navigationTitle;
@end

@implementation ViewController

- (instancetype)initWithMode:(HJDemoMode)mode
                       title:(NSString *)title
                sequencePath:(NSString *)sequencePath
               hideControls:(BOOL)hideControls {
    if (self = [super init]) {
        _demoMode = mode;
        _navigationTitle = [title copy];
        _sequencePath = [sequencePath copy];
        _hideControls = hideControls;
    }
    return self;
}

- (instancetype)init {
    return [self initWithMode:HJDemoModeFaceu
                        title:@"Faceu Demo"
                 sequencePath:@"imgseq/sing"
                hideControls:NO];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
    if (self.navigationTitle.length > 0) {
        self.title = self.navigationTitle;
    }
    [self setupGLView];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    self.glView.frame = self.view.bounds;
}

- (void)setupGLView {
    if (self.glView) {
        return;
    }
    self.glView = [[HJGLView alloc] initWithFrame:self.view.bounds];
    self.glView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.glView.controlsHidden = self.hideControls;
    [self.view addSubview:self.glView];
    [self.glView setResourcePath:@"resource"];
    NSString *sequence = self.sequencePath.length > 0 ? self.sequencePath : @"imgseq/sing";
    if (sequence.length > 0) {
        [self.glView playImageSequenceAtRelativePath:sequence];
    }
}
//
@end
