CXX       	?= g++
TAR		?= bsdtar
PREFIX	  	?= /usr
MANPREFIX	?= $(PREFIX)/share/man
APPPREFIX 	?= $(PREFIX)/share/applications
LOCALEDIR	?= $(PREFIX)/share/locale
VARS  	  	?= -DENABLE_NLS=1 -DWLR_USE_UNSTABLE=1

DEBUG 		?= 1

# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build/debug
        CXXFLAGS := -ggdb3 -Wall -Wextra -Wpedantic -Wno-unused-parameter -DDEBUG=1 $(DEBUG_CXXFLAGS) $(CXXFLAGS)
else
	# Check if an optimization flag is not already set
	ifneq ($(filter -O%,$(CXXFLAGS)),)
    		$(info Keeping the existing optimization flag in CXXFLAGS)
	else
    		CXXFLAGS := -O3 $(CXXFLAGS)
	endif
        BUILDDIR  = build/release
endif

NAME		 = tabwm
TARGET		?= $(NAME)
VERSION    	 = 0.0.1
SRC 	   	 = $(wildcard src/*.cpp)
OBJ 	   	 = $(SRC:.cpp=.o)
LDFLAGS   	+= -lfmt -L/usr/lib64 -lwlroots -lwayland-server -lxkbcommon
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -fvisibility=hidden -Iinclude -I/usr/include/pixman-1 -std=c++20 $(VARS)

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(BUILDDIR)
	$(CXX) $(OBJ) $(LDFLAGS) -o $(BUILDDIR)/$(TARGET) 

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(OBJ)

install: $(TARGET)
	install $(BUILDDIR)/$(TARGET) -Dm 755 -v $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: $(TARGET) install all
