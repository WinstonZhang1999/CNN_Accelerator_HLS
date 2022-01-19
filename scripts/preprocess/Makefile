# Define V=1 for a more verbose compilation
ifndef V
        QUIET_CXX           = @echo '   ' CXX $@;
        QUIET_MKDIR         = @echo '   ' MKDIR $@;
        QUIET_RUN           = @echo '   ' RUN $@;
        QUIET_UNZIP         = @echo '   ' UNZIP $@;
        QUIET_WGET          = @echo '   ' WGET $@;
        QUIET_CLEAN         = @echo '   ' CLEAN $@;
endif

RM = rm -rf

INPUT = $(wildcard *.bmp)
OUTPUT = $(INPUT:.bmp=.bin)

#
# Environment variables
#

OPENCV ?= /opt/opencv

#
# Compile and Link Flags
# Keep the variable name as below. This allows make to automatically infer
# compiler and linker rules.
#

# Include folders for header files
INCDIR = -I$(OPENCV)/include -I../../pv/mojo

# Compile flags
CXXFLAGS ?=  -O3 -Wall -Werror -fmax-errors=5 -std=c++11
CXXFLAGS += $(INCDIR)

# Link flags
LDLIBS = -L$(OPENCV)/lib64

LDFLAGS = \
	-lopencv_videostab \
	-lopencv_photo \
	-lopencv_stitching \
	-lopencv_objdetect \
	-lopencv_video \
	-lopencv_ml \
	-lopencv_calib3d \
	-lopencv_features2d \
	-lopencv_imgcodecs \
	-lopencv_highgui \
	-lopencv_flann \
	-lopencv_imgproc \
	-lopencv_core

# List of source files.
SRCS := preprocess.cpp

# In this Makefile each source produces a corresponding executable.
OBJS := $(SRCS:.cpp=)

# Find all headers for dependency (i.e. recompile if one header changes)
HDRS := $(wildcard ../../pv/mojo/*.h)

# The first target gets executed by default when typing simply make.
all: $(OUTPUT)

# Let make know the suffix of source and object files (if any)
.SUFFIX: .c .cpp .bin .bmp

# Compile
# The following rules are inferred by make. Overriding to add $(QUIET_CXX)
# Try removing the following target and you will still be able to compile!
.cpp:
	$(QUIET_CXX) $(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS) $(LDFLAGS)

# Add header files dependency
$(OBJS): $(HDRS)

# Create a target to clean from compiled objects
clean:
	$(QUIET_CLEAN) $(RM) $(OBJS) *.bin

# Declare as .PHONY all targets that do not generate a file whose name matches
# the name of the target (e.g. make clean won't generate a file named clean,
# while make dwarf will compile an object file named dwarf)
.PHONY: all clean


#
# Run
#

# Note that we add OPENCV/lib to LD_LIBRARY_PATH because libraries are dynamically
# linked when the program is loaded into memory and not at compile time.
$(OUTPUT): %.bin : %.bmp preprocess
	$(QUIET_RUN) LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(OPENCV)/lib64 ./preprocess $<

.PHONY: $(RUNS)
