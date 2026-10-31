#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>

#include "blades.h"
#include "ui_audio.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

static BladesHeld held;
#include "stick.h"
static BladesStick stick;
static BladesHeld keyboardHeld;
static BladesHeld keyPressQueue[32];
static unsigned int keyRead, keyWrite;

static int write_ppm(const char *path)
{
	FILE *out = fopen(path, "wb");
	const uint32_t *pixels = blades_pixels();
	int y, x;
	if (!out) {
		perror(path);
		return 0;
	}
	fprintf(out, "P6\n%d %d\n255\n", BLADES_WIDTH, BLADES_HEIGHT);
	for (y = 0; y < BLADES_HEIGHT; y++) {
		for (x = 0; x < BLADES_WIDTH; x++) {
			uint32_t p = pixels[y * BLADES_WIDTH + x];
			unsigned char rgb[3] = { (p >> 16) & 255, (p >> 8) & 255, p & 255 };
			fwrite(rgb, 1, 3, out);
		}
	}
	{
		int failed = ferror(out);
		if (fclose(out) != 0)
			failed = 1;
		return !failed;
	}
}

static NSSound *uiSounds[5];
static void load_ui_sounds(const char *executable)
{
	NSString *root = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"audio"];
	if (![[NSFileManager defaultManager] fileExistsAtPath:root])
		root = [[[NSString stringWithUTF8String:executable] stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"../assets/audio"];
	NSArray *names = @[@"move", @"blade", @"select", @"guide_open", @"guide_close"];
	for (int i = 0; i < 5; i++) {
		NSString *path = [[root stringByAppendingPathComponent:names[i]] stringByAppendingPathExtension:@"wav"];
		uiSounds[i] = [[NSSound alloc] initWithContentsOfFile:path byReference:NO];
		uiSounds[i].volume = 0.75f;
	}
}
static void play_ui_sound(void)
{
	int cue = blades_take_sound();
	if (!cue) return;
	int index = cue == UI_SOUND_BLADE ? 1 : cue == UI_SOUND_SELECT ? 2 : cue == UI_SOUND_GUIDE_OPEN ? 3 : cue == UI_SOUND_GUIDE_CLOSE ? 4 : 0;
	[uiSounds[index] stop];
	[uiSounds[index] play];
}

static void capture_tap(BladesHeld press)
{
	blades_frame(&press, 1.0f / 60.0f);
	memset(&press, 0, sizeof press);
	for (int i = 0; i < 40; i++) blades_frame(&press, 1.0f / 60.0f);
}

@interface BladesAppDelegate : NSObject <NSApplicationDelegate>
@end
@implementation BladesAppDelegate
- (void)applicationDidResignActive:(NSNotification *)notification
{
	(void)notification;
	memset(&keyboardHeld, 0, sizeof keyboardHeld);
	keyRead = keyWrite = 0;
	memset(&held, 0, sizeof held);
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
	(void)app;
	return YES;
}
@end

@interface BladesView : NSView
@end

@implementation BladesView

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)keyDown:(NSEvent *)event
{
	if (event.isARepeat)
		return;
	[self setKey:event pressed:1];
}

- (void)keyUp:(NSEvent *)event
{
	[self setKey:event pressed:0];
}

- (void)setKey:(NSEvent *)event pressed:(int)pressed
{
	NSString *chars = event.charactersIgnoringModifiers.lowercaseString;
	unichar c = chars.length ? [chars characterAtIndex:0] : 0;
	BladesHeld press = { 0 };
	switch (c) {
	case NSUpArrowFunctionKey:
		keyboardHeld.up = pressed;
		press.up = pressed;
		break;
	case NSDownArrowFunctionKey:
		keyboardHeld.down = pressed;
		press.down = pressed;
		break;
	case NSLeftArrowFunctionKey:
		keyboardHeld.left = pressed;
		press.left = pressed;
		break;
	case NSRightArrowFunctionKey:
		keyboardHeld.right = pressed;
		press.right = pressed;
		break;
	case '\r':
	case '\n':
	case 'a':
		keyboardHeld.a = pressed;
		press.a = pressed;
		break;
	case 27:
	case 'b':
		keyboardHeld.b = pressed;
		press.b = pressed;
		break;
	case '\t':
	case 'g':
		keyboardHeld.guide = pressed;
		press.guide = pressed;
		break;
	case 'x':
		keyboardHeld.x = pressed;
		press.x = pressed;
		break;
	case 'y':
		keyboardHeld.y = pressed;
		press.y = pressed;
		break;
	default:
		break;
	}
	if (pressed && (press.up || press.down || press.left || press.right || press.a || press.b || press.y || press.x || press.guide)) {
		keyPressQueue[keyWrite++ % 32] = press;
		if (keyWrite - keyRead > 32) keyRead = keyWrite - 32;
	}

}

- (void)drawRect:(NSRect)dirty
{
	const uint32_t *pixels = blades_pixels();
	CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
	CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, pixels, BLADES_WIDTH * BLADES_HEIGHT * 4, NULL);
	CGImageRef image = CGImageCreate(BLADES_WIDTH, BLADES_HEIGHT, 8, 32, BLADES_WIDTH * 4, space,
		kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little, provider, NULL, false, kCGRenderingIntentDefault);
	(void)dirty;
	CGContextDrawImage(ctx, CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height), image);
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(space);
}

@end

static void poll_pad(void)
{
	held = keyboardHeld;
	GCExtendedGamepad *pad = nil;
	for (GCController *controller in GCController.controllers) {
		if (controller.extendedGamepad) {
			pad = controller.extendedGamepad;
			break;
		}
	}
	if (!pad) {
		stick.direction = 0;
		return;
	}
	held.up = held.up || pad.dpad.up.pressed;
	held.down = held.down || pad.dpad.down.pressed;
	held.left = held.left || pad.dpad.left.pressed;
	held.right = held.right || pad.dpad.right.pressed;
	blades_stick_apply(&held, &stick, (int)(pad.leftThumbstick.xAxis.value*32767), (int)(pad.leftThumbstick.yAxis.value*32767));
	held.a = held.a || pad.buttonA.pressed;
	held.b = held.b || pad.buttonB.pressed;
	held.y = held.y || pad.buttonY.pressed;
	held.x = held.x || pad.buttonX.pressed;
	held.guide = held.guide || pad.buttonMenu.pressed;
}

int main(int argc, const char **argv)
{
	@autoreleasepool {
		blades_init();
		if (argc > 1 && strcmp(argv[1], "--dump") == 0) {
			const char *path = argc > 2 ? argv[2] : "blades.ppm";
			const char *state = argc > 3 ? argv[3] : "games";
			if (strncmp(state, "guide", 5) == 0) {
				if (strcmp(state, "guide-game") == 0) {
					capture_tap((BladesHeld){ .a = 1 });
					capture_tap((BladesHeld){ .a = 1 });
					capture_tap((BladesHeld){ .down = 1 });
				}
				blades_set_clock(4, 8); /* Fixed only for reproducible reference captures. */
				if (strcmp(state, "guide-mid") == 0) blades_frame(&(BladesHeld){ .guide = 1 }, .075f);
				else {
					capture_tap((BladesHeld){ .guide = 1 });
					if (strcmp(state, "guide-signin") == 0) capture_tap((BladesHeld){ .x = 1 });
					else if (strcmp(state, "guide-confirm") == 0) capture_tap((BladesHeld){ .y = 1 });
					else if (strcmp(state, "guide-info") == 0) {
						capture_tap((BladesHeld){ .down = 1 });
						capture_tap((BladesHeld){ .a = 1 });
					} else if (strcmp(state, "guide") != 0 && strcmp(state, "guide-game") != 0) {
						fprintf(stderr, "Unknown capture state: %s\n", state); return 2;
					}
				}
			} else if (strcmp(state, "market") == 0) {
				capture_tap((BladesHeld){ .left = 1 });
				capture_tap((BladesHeld){ .left = 1 });
			} else if (strcmp(state, "live") == 0) capture_tap((BladesHeld){ .left = 1 });
			else if (strcmp(state, "media") == 0) capture_tap((BladesHeld){ .right = 1 });
			else if (strcmp(state, "system") == 0 || strcmp(state, "console") == 0 || strcmp(state, "display") == 0 || strcmp(state, "audio") == 0) {
				capture_tap((BladesHeld){ .right = 1 });
				capture_tap((BladesHeld){ .right = 1 });
				if (strcmp(state, "system") != 0) capture_tap((BladesHeld){ .a = 1 });
				if (strcmp(state, "display") == 0) capture_tap((BladesHeld){ .a = 1 });
				if (strcmp(state, "audio") == 0) {
					capture_tap((BladesHeld){ .down = 1 }); capture_tap((BladesHeld){ .a = 1 });
				}
			} else if (strcmp(state, "library") == 0 || strcmp(state, "my-games") == 0 || strcmp(state, "second-game") == 0) {
				capture_tap((BladesHeld){ .a = 1 });
				if (strcmp(state, "library") != 0) capture_tap((BladesHeld){ .a = 1 });
				if (strcmp(state, "second-game") == 0) capture_tap((BladesHeld){ .down = 1 });
			} else if (strcmp(state, "games") != 0) {
				fprintf(stderr, "Unknown capture state: %s\n", state); return 2;
			}
			return write_ppm(path) ? 0 : 1;
		}
		[NSApplication sharedApplication];
		load_ui_sounds(argv[0]);
		BladesAppDelegate *delegate = [[BladesAppDelegate alloc] init];
		NSApp.delegate = delegate;
		NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Blades"];
		NSMenuItem *appItem = [[NSMenuItem alloc] initWithTitle:@"Blades" action:nil keyEquivalent:@""];
		NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"Blades"];
		[appMenu addItemWithTitle:@"Quit Blades" action:@selector(terminate:) keyEquivalent:@"q"];
		appItem.submenu = appMenu;
		[menu addItem:appItem];
		NSApp.mainMenu = menu;
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		NSRect rect = NSMakeRect(80, 80, BLADES_WIDTH, BLADES_HEIGHT);
		NSWindow *window = [[NSWindow alloc] initWithContentRect:rect
			styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
			backing:NSBackingStoreBuffered
			defer:NO];
		window.title = @"Blades — arrows move, Return selects, Esc goes back, Tab opens Guide";
		BladesView *view = [[BladesView alloc] initWithFrame:rect];
		window.contentView = view;
		[window makeFirstResponder:view];
		[window makeKeyAndOrderFront:nil];
		[NSApp activateIgnoringOtherApps:YES];
		__block NSTimeInterval lastFrame = NSProcessInfo.processInfo.systemUptime;
		[NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0 repeats:YES block:^(NSTimer *timer) {
			(void)timer;
			poll_pad();
			time_t wallTime = time(NULL);
			struct tm *local = localtime(&wallTime);
			if (local) blades_set_clock(local->tm_hour, local->tm_min);
			NSTimeInterval now = NSProcessInfo.processInfo.systemUptime;
			if (keyRead != keyWrite) {
				BladesHeld press = keyPressQueue[keyRead++ % 32];
				BladesHeld released = held, combined = held;
				if (press.up) { released.up = 0; combined.up = 1; }
				if (press.down) { released.down = 0; combined.down = 1; }
				if (press.left) { released.left = 0; combined.left = 1; }
				if (press.right) { released.right = 0; combined.right = 1; }
				if (press.a) { released.a = 0; combined.a = 1; }
				if (press.b) { released.b = 0; combined.b = 1; }
				if (press.y) { released.y = 0; combined.y = 1; }
				if (press.x) { released.x = 0; combined.x = 1; }
				if (press.guide) { released.guide = 0; combined.guide = 1; }
				blades_frame(&released, 0);
				blades_frame(&combined, (float)(now - lastFrame));
			} else blades_frame(&held, (float)(now - lastFrame));
			lastFrame = now;
			play_ui_sound();
			[view setNeedsDisplay:YES];
		}];
		[NSApp run];
	}
	return 0;
}
