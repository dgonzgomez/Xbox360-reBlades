CC = clang
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -I src -I third_party
FRAMEWORKS = -framework Cocoa -framework GameController -framework CoreGraphics
SRCS = src/draw.c src/font.c src/app.c src/library.c src/guide.c src/guide_art_data.c src/asset_codec.c src/font_data.c src/reference_assets.c src/reference_asset_data.c platform/mac/main.m
OUT = build/blades-preview
C_SRCS = $(filter %.c,$(SRCS))
MAC_OBJS = $(patsubst %.c,build/mac/%.o,$(C_SRCS)) build/mac/platform/mac/main.o
CHECK_OBJS = $(patsubst %.c,build/check/%.o,$(C_SRCS)) build/check/tests/navigation.o
SANITIZERS = -g -fsanitize=address,undefined

.PHONY: preview dump check assets app guide-assets check-xsdk-layout

preview: $(OUT)

$(OUT): $(MAC_OBJS)
	$(CC) $(MAC_OBJS) $(FRAMEWORKS) -o $(OUT)

build/mac/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/mac/%.o: %.m
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/check/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SANITIZERS) -MMD -MP -c $< -o $@

dump: $(OUT)
	$(OUT) --dump build/games-home.ppm games
	$(OUT) --dump build/market.ppm market
	$(OUT) --dump build/live.ppm live
	$(OUT) --dump build/media.ppm media
	$(OUT) --dump build/system.ppm system
	$(OUT) --dump build/games-library.ppm library
	$(OUT) --dump build/my-games.ppm my-games
	$(OUT) --dump build/games-selected.ppm second-game
	$(OUT) --dump build/console.ppm console
	$(OUT) --dump build/display.ppm display
	$(OUT) --dump build/audio.ppm audio
	$(OUT) --dump build/guide.ppm guide
	$(OUT) --dump build/guide-mid.ppm guide-mid
	$(OUT) --dump build/guide-signin.ppm guide-signin
	$(OUT) --dump build/guide-confirm.ppm guide-confirm
	$(OUT) --dump build/guide-info.ppm guide-info
	$(OUT) --dump build/guide-game.ppm guide-game

assets:
	python3 tools/bake_widescreen.py

build/navigation-check: $(CHECK_OBJS)
	$(CC) $(SANITIZERS) $(CHECK_OBJS) -lm -o $@

build/font-check: tests/font.c src/font.c src/draw.c src/font_data.c
	$(CC) $(CFLAGS) $(SANITIZERS) $^ -lm -o $@

build/library-check: tests/library.c src/library.c
	$(CC) $(CFLAGS) $(SANITIZERS) $^ -o $@

check: build/navigation-check build/font-check build/library-check
	build/navigation-check
	build/font-check
	build/library-check

# Build Xbox with CMake first; inspect its compiled shader using the host compiler.
check-xsdk-layout: build/xsdk/blades_shaders.h platform/xsdk/screen_layout.h tests/xsdk_layout.c
	$(CC) -Wall -Wextra -Werror -I build/xsdk -I platform/xsdk -I ../blades-project/research/xsdk/include tests/xsdk_layout.c -o build/xsdk-layout-check
	build/xsdk-layout-check

app: $(OUT)
	mkdir -p build/Blades.app/Contents/MacOS
	cp $(OUT) build/Blades.app/Contents/MacOS/blades-preview.next
	mv build/Blades.app/Contents/MacOS/blades-preview.next build/Blades.app/Contents/MacOS/blades-preview
	cp platform/mac/Info.plist build/Blades.app/Contents/Info.plist
	mkdir -p build/Blades.app/Contents/Resources/audio
	cp assets/audio/*.wav build/Blades.app/Contents/Resources/audio/

-include $(MAC_OBJS:.o=.d) $(CHECK_OBJS:.o=.d)

guide-assets:
	python3 tools/bake_guide.py
