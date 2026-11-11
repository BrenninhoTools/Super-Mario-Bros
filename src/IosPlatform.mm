#include "IosPlatform.hpp"

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <SDL_syswm.h>

@interface SmbRomPickerDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic, copy) NSString* destination;
@end

@implementation SmbRomPickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    NSURL* url = urls.firstObject;
    if (!url)
    {
        return;
    }
    BOOL scoped = [url startAccessingSecurityScopedResource];
    NSData* data = [NSData dataWithContentsOfURL:url];
    if (scoped)
    {
        [url stopAccessingSecurityScopedResource];
    }
    if (!data || data.length > 4 * 1024 * 1024)
    {
        return;
    }
    NSFileManager* files = [NSFileManager defaultManager];
    NSString* folder = [self.destination stringByDeletingLastPathComponent];
    [files createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:nil];
    NSString* temp = [self.destination stringByAppendingString:@".tmp"];
    if ([data writeToFile:temp atomically:YES])
    {
        [files removeItemAtPath:self.destination error:nil];
        [files moveItemAtPath:temp toPath:self.destination error:nil];
    }
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
}

@end

namespace
{
SmbRomPickerDelegate* pickerDelegate = nil;

UIViewController* rootController(SDL_Window* window)
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!window || !SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_UIKIT)
    {
        return nil;
    }
    UIViewController* controller = info.info.uikit.window.rootViewController;
    while (controller.presentedViewController)
    {
        controller = controller.presentedViewController;
    }
    return controller;
}
}

namespace smb
{
void iosRequestRom(SDL_Window* window, const std::string& destination)
{
    UIViewController* controller = rootController(window);
    if (!controller || [controller isKindOfClass:[UIDocumentPickerViewController class]])
    {
        return;
    }
    if (!pickerDelegate)
    {
        pickerDelegate = [[SmbRomPickerDelegate alloc] init];
    }
    pickerDelegate.destination = [NSString stringWithUTF8String:destination.c_str()];
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:YES];
    picker.delegate = pickerDelegate;
    picker.allowsMultipleSelection = NO;
    picker.modalPresentationStyle = UIModalPresentationFormSheet;
    [controller presentViewController:picker animated:YES completion:nil];
}

std::string iosDocumentsDirectory()
{
    NSArray<NSString*>* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if (paths.count == 0)
    {
        return std::string();
    }
    return std::string([paths.firstObject UTF8String]) + "/";
}

void iosSafeAreaInsets(SDL_Window* window, int outputWidth, float& left, float& right, float& bottom)
{
    left = right = bottom = 0.0f;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!window || !SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_UIKIT)
    {
        return;
    }
    UIWindow* uiWindow = info.info.uikit.window;
    CGFloat width = uiWindow.bounds.size.width;
    if (width <= 0)
    {
        return;
    }
    UIEdgeInsets insets = uiWindow.safeAreaInsets;
    float scale = (float)(outputWidth / width);
    left = (float)insets.left * scale;
    right = (float)insets.right * scale;
    bottom = (float)insets.bottom * scale;
}
}
