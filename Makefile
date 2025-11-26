# Makefile for tree.cpp - maximum compression + UPX

CXX = clang++
SRC = src/main.cpp
TARGET = build/tree.exe

# Clang++ flags: max optimization for size and speed, remove debug, dead code elimination
CXXFLAGS = -std=c++20 -Os -flto -ffunction-sections -fdata-sections -Wl,--gc-sections -s

preparedir: 
	@mkdir -p build/

all: preparedir $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "[*] Stripping symbols..."
	strip --strip-all $(TARGET) || true
	@echo "[*] Compressing with UPX (max)..."
	upx --best --ultra-brute $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean preparedir
