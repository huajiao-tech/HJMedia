#import "MainMenuViewController.h"

#import "SRViewController.h"
#import "ViewController.h"

static NSString *const kMenuCellIdentifier = @"MenuCell";

@interface MainMenuViewController ()
@property (nonatomic, copy) NSArray<NSDictionary<NSString *, NSString *> *> *items;
@end

@implementation MainMenuViewController

- (instancetype)init {
    return [self initWithStyle:UITableViewStyleInsetGrouped];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"HJMedia 示例";
    self.items = @[
        @{ @"title": @"Faceu 特效", @"detail": @"添加/移除 Faceu 特效并查看效果", @"type": @"faceu" },
        @{ @"title": @"SR超分", @"detail": @"超分示例入口（UI骨架）", @"type": @"sr" }
    ];
    [self.tableView registerClass:UITableViewCell.class forCellReuseIdentifier:kMenuCellIdentifier];
    self.tableView.rowHeight = 64.0;
    self.tableView.tableFooterView = [[UIView alloc] initWithFrame:CGRectZero];
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.items.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kMenuCellIdentifier];
    if (cell.detailTextLabel == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:kMenuCellIdentifier];
    }
    NSDictionary *item = self.items[indexPath.row];
    cell.textLabel.text = item[@"title"];
    cell.detailTextLabel.text = item[@"detail"];
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    return cell;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSDictionary *item = self.items[indexPath.row];
    NSString *type = item[@"type"];
    UIViewController *controller = nil;
    if ([type isEqualToString:@"faceu"]) {
        controller = [[ViewController alloc] initWithMode:HJDemoModeFaceu
                                                    title:@"Faceu 特效"
                                             sequencePath:@"imgseq/sing"
                                            hideControls:NO];
    } else if ([type isEqualToString:@"sr"]) {
        controller = [[SRViewController alloc] init];
    }
    if (!controller) {
        [tableView deselectRowAtIndexPath:indexPath animated:YES];
        return;
    }
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    controller.view.backgroundColor = [UIColor blackColor];
    if (self.navigationController) {
        [self.navigationController pushViewController:controller animated:YES];
    } else {
        [self presentViewController:controller animated:YES completion:nil];
    }
}

@end
