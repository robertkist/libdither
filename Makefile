# default MingW path for Qt 6.3.x:
WIN_MINGW_BIN_PATH=C:\\Qt\\Tools\\mingw1120_64\\bin

LIB_VERSION=2025.07.07a

LIBNAME=libdither
SRCDIR=src/$(LIBNAME)
OBJDIR=build
DISTDIR=dist

SRC=libdither.c ditherimage.c random.c gamma.c queue.c dither_dbs.c dither_dotdiff.c \
    dither_errordiff.c dither_kallebach.c dither_ordered.c dither_riemersma.c dither_threshold.c \
	dither_varerrdiff.c dither_pattern.c dither_dotlippens.c dither_grid.c \
	color_bytepalette.c color_floatcolor.c color_models.c color_quant_mediancut.c color_cachedpalette.c \
	color_colorimage.c color_floatpalette.c color_bytecolor.c color_quant_wu.c color_quant_kdtree.c \
	kdtree/kdtree.c tetrapal/tetrapal.c

OBJ=$(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
OBJFILES=$(patsubst %.c, %.o, $(SRC))

CFLAGS=-std=c11 -Wall -Wextra -Wconversion -Wshadow -Wstrict-overflow -Wformat=2 -Wundef -fno-common -O3 -Os \
		        -Wpedantic -pedantic -Werror -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -D"LIB_VERSION=\"$(LIB_VERSION)\""

ifdef OS  # Windows:
define fn_mkdir
	if not exist "$(1)" mkdir "$(1)"
endef
	CP=copy
	DELTREE=rd /s /q
	CC=SET PATH=$(WIN_MINGW_BIN_PATH);%PATH% && gcc
	LIBEXT=dll
	SEP=\\
else  # Unix based platforms
define fn_mkdir
	@mkdir -p "$(1)"
endef
	CP=cp
	DELTREE=rm -Rf
	SEP=/
	ifeq ($(shell uname), Darwin)  # macOS
		CC=clang
		LIBEXT=dylib
	else  # other Unix / Linux
		CC=gcc
		LIBEXT=so
		UNIXFLAGS=-Wl,-R.
		CFLAGS+=-Wno-type-limits
	endif
endif

libdither: TARGETARCH=
libdither_x64: TARGETARCH=-target x86_64-apple-macos10.12
libdither_arm64: TARGETARCH=-target arm64-apple-macos11

.PHONY: all
all:
	@echo "Targets:"
	@echo "* libdither - build library for current platform (gcc/clang/MingW)"
	@echo "* libdither_x64 - build macOS x86 library"
	@echo "* libdither_arm64 - build macOS apple silicon library"
	@echo "* libdither_universal - build universal macOS library"
	@echo "* libdither_msvc - build using MSVC on Windows"
	@echo "* demo - builds a small executable for the current platform to demo libdither's capabilities"
	@echo "* clean"

$(LIBNAME)_universal:
	make $(LIBNAME)_arm64
	$(call fn_mkdir,$(DISTDIR)/arm64)
	mv $(DISTDIR)/$(LIBNAME).$(LIBEXT) $(DISTDIR)/arm64/$(LIBNAME).$(LIBEXT)
	-$(DELTREE) $(OBJDIR)
	make $(LIBNAME)_x64
	$(call fn_mkdir,$(DISTDIR)/x64)
	mv $(DISTDIR)/$(LIBNAME).$(LIBEXT) $(DISTDIR)/x64/$(LIBNAME).$(LIBEXT)
	cd $(DISTDIR) && lipo x64/$(LIBNAME).$(LIBEXT) arm64/$(LIBNAME).$(LIBEXT) -output $(LIBNAME).$(LIBEXT) -create

$(LIBNAME)_x64: $(LIBNAME)_build

$(LIBNAME)_arm64: $(LIBNAME)_build

$(LIBNAME): $(LIBNAME)_build

$(LIBNAME)_build: $(OBJDIR)/$(LIBNAME).$(LIBEXT)
	$(call fn_mkdir,$(DISTDIR))
	$(CP) $(OBJDIR)$(SEP)$(LIBNAME).$(LIBEXT) $(DISTDIR)$(SEP)$(LIBNAME).$(LIBEXT)
	@echo "$(LIBNAME) build successfully $(TARGETARCH)"

$(OBJDIR)/$(LIBNAME).$(LIBEXT): $(OBJ)
	cd $(OBJDIR) && $(CC) $(TARGETARCH) -shared $(OBJFILES) -fPIC -o $(LIBNAME).$(LIBEXT)

$(OBJDIR)/%.o: $(addprefix $(SRCDIR)/, %.c)
	$(call fn_mkdir,$(OBJDIR))
	$(call fn_mkdir,$(OBJDIR)/tetrapal)
	$(call fn_mkdir,$(OBJDIR)/kdtree)
	$(CC) $(CFLAGS) $(TARGETARCH) -c -fPIC -c $< -o $@

libdither_msvc:
	@echo Make sure to run vcvars64.bat or this target will fail...
	$(call fn_mkdir,$(OBJDIR))
	cd build && cl.exe ..\\src\\libdither\\*.c /LD /DLL /Fe: libdither.dll
	cd build && cl.exe ..\\src\\demo\\demo.c ..\src\demo\spng.c libdither.lib /I..\\src\\libdither /L. /Fe: demo.exe
	$(call fn_mkdir,$(DISTDIR))
	copy src\\demo\\*.bmp $(DISTDIR)
	copy build\\demo.exe $(DISTDIR)
	copy build\\libdither.dll $(DISTDIR)

demo: libdither
	cd dist && $(CC) $(UNIXFLAGS) -I../src/libdither -L. ../src/demo/demo.c ../src/demo/lodepng.c -ldither -lm -o demo
	$(CP) src$(SEP)demo$(SEP)*.png $(DISTDIR)
	$(CP) src$(SEP)demo$(SEP)blue_noise.bmp $(DISTDIR)

demo_run: demo
	@cd dist && ./demo
	#open -a Preview	dist/david_OUT.png

.PHONY: clean
clean:
	-@$(DELTREE) $(DISTDIR)
	-@$(DELTREE) $(OBJDIR)
