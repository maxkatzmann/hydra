CC := g++ # This is the main compiler
SRCDIR := src
BUILDDIR := build
TARGET := bin/hydra

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS := -std=c++17 -stdlib=libc++ -O0 -DDEBUG -g -w -Wall
else
	CFLAGS := -std=c++17 -stdlib=libc++ -O3 -w -Wall
endif

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name "*.$(SRCEXT)")
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

INC := -I include -I /usr/local/include -L/usr/local/lib -L /opt/local/lib

UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		LIB := -lgflags -lglog
	endif
	ifeq ($(UNAME_S),Darwin)
		LIB := -lgflags -lglog
	endif

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB) $(INC)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@mkdir -p bin
	@echo " $(CC) $(CFLAGS) $(INC) $(LIB) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

debug:
	$(MAKE) $(MAKEFILE) DEBUG=1

release:
	$(MAKE) $(MAKEFILE) DEBUG=0

.PHONY: clean
