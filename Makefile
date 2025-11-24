SFML_ROOT ?= $(HOME)/SFML-2.5.0      # your known-good 2.5.0 build
INCDIR     = $(SFML_ROOT)/include
LIBDIR     = $(SFML_ROOT)/build-x86_64/lib

CXX      := c++
CXXFLAGS := -std=c++17 -arch x86_64 -I. -I$(INCDIR)
LDFLAGS  := -L$(LIBDIR) -Wl,-rpath,$(LIBDIR)
LDLIBS   := -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

SRC      := main.cpp AtmInterface.cpp
TARGET   := atm_simulator

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)