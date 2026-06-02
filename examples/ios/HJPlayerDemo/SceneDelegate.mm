#import "SceneDelegate.h"

#import "ViewController.h"

@implementation SceneDelegate

- (void)scene:(UIScene *)scene
willConnectToSession:(UISceneSession *)session
      options:(UISceneConnectionOptions *)connectionOptions {
    (void)session;
    (void)connectionOptions;

    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }

    UIWindowScene *window_scene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:window_scene];
    ViewController *view_controller = [[ViewController alloc] init];
    UINavigationController *navigation_controller = [[UINavigationController alloc] initWithRootViewController:view_controller];
    self.window.rootViewController = navigation_controller;
    [self.window makeKeyAndVisible];
}

@end
