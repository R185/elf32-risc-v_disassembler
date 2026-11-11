CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
TARGET   ?= disasm
SRCS      = main.cpp
HDRS      = constants.cpp elf_headers.cpp decode.cpp

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
