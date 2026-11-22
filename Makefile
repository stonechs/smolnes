.PHONY: all clean
all: smolnes deobfuscated

WARN=-std=c99 \
		 -Wall \
     -Wno-parentheses \
		 -Wno-misleading-indentation \
		 -Wno-bool-operation \
		 -Wno-discarded-qualifiers \
		 -Wno-incompatible-pointer-types-discards-qualifiers \
		 -Wno-unknown-warning-option \
		 -Wno-switch-outside-range \
		 -Wno-unused-value \
		 -Wno-char-subscripts \
		 -Wno-switch
SDLFLAGS=$(shell pkg-config --cflags --libs sdl2)
SDL_SOURCE_DIR=archive/V3-baselines/root-sdl-original-2026-11-15/source

smolnes: $(SDL_SOURCE_DIR)/smolnes.c
	$(CC) -O2 -o $@ $< ${SDLFLAGS} -g ${WARN}

deobfuscated: $(SDL_SOURCE_DIR)/deobfuscated.c
	$(CC) -O2 -o $@ $< ${SDLFLAGS} -g ${WARN}

clean:
	rm -f smolnes deobfuscated
