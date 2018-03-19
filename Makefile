# Header files
GLMDIR = libs/glm
HDIR := include $(GLMDIR)

# Source files
SRCDIR = src
SRCS = $(wildcard src/*.cpp)

# Dependencies
DEPDIR = dependencies
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

# Object files
BUILDDIR = build
OBJS = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%.o,$(basename $(SRCS)))

# Binary files
BINDIR = bin
BINARY_RAYTRACER = $(BINDIR)/raytracer
BINARY_RASTERISER = $(BINDIR)/rasteriser

# Compilation options
CXX = icpc
CXXFLAGS = -std=c++11 -Wall -Ofast -march=native -ggdb -g3 -D unix -D GLM_FORCE_SSE2 -D GLM_FORCE_ALIGNED -D textureLess $(addprefix -I, $(HDIR)) $(shell sdl2-config --cflags) $(DEPFLAGS)
COMPILE = $(CXX) -o $@ -c $< $(CXXFLAGS)

# Link Options
LDFLAGS += $(shell sdl2-config --libs) -fopenmp
LINK_RAYTRACER = $(CXX) -o $@ $(filter-out $(BUILDDIR)/rasteriser.o, $^) $(LDFLAGS) $(LDLIBS)
LINK_RASTERISER = $(CXX) -o $@ $(filter-out $(BUILDDIR)/raytracer.o, $^) $(LDFLAGS) $(LDLIBS)

.PHONY: all clean
all: $(BUILDDIR) $(DEPDIR) $(BINDIR) $(BINARY_RAYTRACER) $(BINARY_RASTERISER)
clean:
	@$(RM) $(BUILDDIR)/*.o $(DEPDIR)/*.d $(BINARY) screenshot.bmp

$(BUILDDIR) $(DEPDIR) $(BINDIR):
	mkdir -p $@

# Compiling Objects
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(info $@)
	@$(COMPILE)

# Compiling Release Binary
$(BINARY_RAYTRACER): $(OBJS)
	$(info $@)
	@$(LINK_RAYTRACER)

$(BINARY_RASTERISER): $(OBJS)
	$(info $@)
	@$(LINK_RASTERISER)

include $(wildcard $(DEPDIR)/*.d)