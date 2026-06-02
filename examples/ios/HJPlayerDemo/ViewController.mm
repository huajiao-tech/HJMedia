#import "ViewController.h"

#import "MusicPlayerViewController.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.title = @"HJPlayerDemo";
    self.view.backgroundColor = [UIColor systemBackgroundColor];
    UILabel *titleLabel = [[UILabel alloc] init];
    titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    titleLabel.text = @"Examples";
    titleLabel.font = [UIFont boldSystemFontOfSize:28.0];

    UILabel *subtitleLabel = [[UILabel alloc] init];
    subtitleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    subtitleLabel.text = @"Use this page as the main entry for HJPlayerDemo examples.";
    subtitleLabel.font = [UIFont systemFontOfSize:16.0];
    subtitleLabel.textColor = [UIColor secondaryLabelColor];
    subtitleLabel.numberOfLines = 0;

    UIButton *musicPlayerButton = [UIButton buttonWithType:UIButtonTypeSystem];
    musicPlayerButton.translatesAutoresizingMaskIntoConstraints = NO;
    [musicPlayerButton setTitle:@"Open Music Player Demo" forState:UIControlStateNormal];
    musicPlayerButton.titleLabel.font = [UIFont boldSystemFontOfSize:18.0];
    [musicPlayerButton addTarget:self
                          action:@selector(openMusicPlayerDemo)
                forControlEvents:UIControlEventTouchUpInside];
    musicPlayerButton.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
    musicPlayerButton.backgroundColor = [UIColor secondarySystemBackgroundColor];
    musicPlayerButton.layer.cornerRadius = 12.0;

    UILabel *footerLabel = [[UILabel alloc] init];
    footerLabel.translatesAutoresizingMaskIntoConstraints = NO;
    footerLabel.text = @"Add future demos here and keep ViewController as the only navigation hub.";
    footerLabel.font = [UIFont systemFontOfSize:13.0];
    footerLabel.textColor = [UIColor tertiaryLabelColor];
    footerLabel.numberOfLines = 0;

    UIStackView *stackView = [[UIStackView alloc] initWithArrangedSubviews:@[
        titleLabel,
        subtitleLabel,
        musicPlayerButton,
        footerLabel
    ]];
    stackView.translatesAutoresizingMaskIntoConstraints = NO;
    stackView.axis = UILayoutConstraintAxisVertical;
    stackView.spacing = 18.0;

    [self.view addSubview:stackView];
    [NSLayoutConstraint activateConstraints:@[
        [stackView.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:20.0],
        [stackView.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-20.0],
        [stackView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:24.0]
    ]];
}

- (void)openMusicPlayerDemo {
    MusicPlayerViewController *controller = [[MusicPlayerViewController alloc] init];
    [self.navigationController pushViewController:controller animated:YES];
}

@end
