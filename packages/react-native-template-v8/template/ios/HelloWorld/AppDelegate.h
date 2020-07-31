#import <React/RCTBridgeDelegate.h>
#import <React/RCTCxxBridgeDelegate.h>
#import <UIKit/UIKit.h>

@interface AppDelegate : UIResponder <
  UIApplicationDelegate,
  RCTBridgeDelegate,
  RCTCxxBridgeDelegate>

@property (nonatomic, strong) UIWindow *window;

@end
