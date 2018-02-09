# Header files
GLMDIR = deps/glm
HDIR := include $(GLMDIR)

# Source files
SRCDIR = src
SRCS = $(wildcard $(SRCDIR)/*.cpp)

# Dependencies
DEPDIR = dependencies
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

# Object files
BUILDDIR = build
OBJS = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%.o,$(basename $(SRCS)))

# Binary files
BINDIR = bin
BINARYNAME = computer_graphics
BINARY = $(BINDIR)/$(BINARYNAME)

# Compilation options
CXX = icpc
CXXFLAGS = -std=c++11 -Wall -Ofast -march=native -fopenmp -ggdb -g3 -D unix -D GLM_FORCE_SSE2 -D GLM_FORCE_ALIGNED -D textureLess $(addprefix -I, $(HDIR)) $(shell sdl2-config --cflags) $(DEPFLAGS)
COMPILE = $(CXX) -o $@ -c $< $(CXXFLAGS)

# Link Options
LDFLAGS += $(shell sdl2-config --libs) -fopenmp
LINK = $(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

.PHONY: all clean
all: $(BUILDDIR) $(DEPDIR) $(BINDIR) $(BINARY)
clean:
	@$(RM) $(BUILDDIR)/*.o $(DEPDIR)/*.d $(BINARY) screenshot.bmp

$(BUILDDIR) $(DEPDIR) $(BINDIR):
	mkdir -p $@

# Compiling Objects
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(info $@)
	@$(COMPILE)

# Compiling Release Binary
$(BINARY): $(OBJS)
	$(info $@)
	@$(LINK)

include $(wildcard $(DEPDIR)/*.d)