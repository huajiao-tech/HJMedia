#import "SceneDelegate.h"
#import "MainMenuViewController.h"

@implementation SceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    MainMenuViewController *menu = [[MainMenuViewController alloc] init];
    UINavigationController *navigationController = [[UINavigationController alloc] initWithRootViewController:menu];
    self.window.rootViewController = navigationController;
    [self.window makeKeyAndVisible];
}

@end
