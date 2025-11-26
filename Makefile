# ====== CONFIG ======
CXX = clang++
SRC = src/main.cpp
TARGET = build/tree.exe
BUILD ?= balanced  # default if user runs just 'make'

# ====== BUILD PROFILES ======

# ultra lightweight
CXXFLAGS_tiny = -std=c++20 -Oz -flto \
	-fno-rtti \
	-ffunction-sections -fdata-sections \
	-Wl,--gc-sections \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-fshort-enums \
	-s

# ultra fast
CXXFLAGS_fast = -std=c++20 -O3 -flto -march=native \
	-funroll-loops -fno-math-errno -fno-trapping-math \
	-ffast-math

# balanced - small size + good speed
CXXFLAGS_balanced = -std=c++20 -O2 -flto -march=native \
	-ffunction-sections -fdata-sections \
	-Wl,--gc-sections \
	-fno-rtti \
	-funroll-loops

# ====== MODE SELECTOR ======
ifeq ($(BUILD),tiny)
CXXFLAGS = $(CXXFLAGS_tiny)
else ifeq ($(BUILD),fast)
CXXFLAGS = $(CXXFLAGS_fast)
else ifeq ($(BUILD),balanced)
CXXFLAGS = $(CXXFLAGS_balanced)
else
CXXFLAGS = $(CXXFLAGS_balanced)
endif

# ====== TARGETS ======

all: $(TARGET)

preparedir:
	@mkdir -p build/

$(TARGET): preparedir $(SRC)
	@echo "[*] building in $(BUILD) mode"
	@echo "[*] using flags: $(CXXFLAGS)"
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)
ifeq ($(BUILD),tiny)
	@echo "[*] strip --strip-all"
	@strip --strip-all $(TARGET) 2>/dev/null || true
	@echo "[*] UPX compression..."
	@upx --best --ultra-brute --lzma $(TARGET) 2>/dev/null || echo "[!] UPX not available, skipping compression"
endif
ifeq ($(BUILD),fast)
	@echo "[*] strip --strip-all"
	@strip --strip-all $(TARGET) 2>/dev/null || true
	@echo "[*] UPX compression..."
	@upx --best --ultra-brute --lzma $(TARGET) 2>/dev/null || echo "[!] UPX not available, skipping compression"
endif
ifeq ($(BUILD),balanced)
	@echo "[*] strip --strip-all"
	@strip --strip-all $(TARGET) 2>/dev/null || true
	@echo "[*] strip --strip-unneeded"
	@strip --strip-unneeded $(TARGET) 2>/dev/null || true
endif
	@echo "[✓] Build complete: $(TARGET)"

clean:
	rm -f $(TARGET)

# Build different profiles
tiny:
	@$(MAKE) BUILD=tiny

fast:
	@$(MAKE) BUILD=fast

balanced:
	@$(MAKE) BUILD=balanced

.PHONY: all preparedir clean tiny fast balanced