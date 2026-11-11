CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
TARGET   ?= disasm
SRCS      = main.cpp
HDRS      = lib/constants.cpp lib/elf_headers.cpp lib/decode.cpp

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
