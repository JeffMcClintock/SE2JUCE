# Copyright 2004-2005 by Tim Hockin and Jeff McClintock

TARGET = ExampleHost
SRCS = main.cpp ExampleHost.cpp
OBJS = $(SRCS:.cpp=.o)

TOPDIR = ../..
SDKDIR = $(TOPDIR)/sdk

INCLUDES += -I$(TOPDIR)/api
INCLUDES += -I$(SDKDIR)/lib/include
INCLUDES += -I$(SDKDIR)/common/include
INCLUDES += -I$(SDKDIR)/host/include

DEFS = -DGMPI_TRACE_PREFIX="\"TRACE HOST\""

LDLIBS += -L$(SDKDIR)/host -lgmpi_host_sdk
LDLIBS += -L$(SDKDIR)/lib -lgmpi
LDLIBS += -ldl

CXXFLAGS = -Wall -Werror $(INCLUDES) $(DEFS) $(_DEBUG)
LDFLAGS =

ifneq ($(strip $(DEBUG)),)
_DEBUG = -ggdb
endif

# Set CC to $(CXX) to force the build to cal the C++ compiler for linking.
CC=$(CXX)

all: $(TARGET)

$(TARGET): $(OBJS)

clean:
	$(RM) $(OBJS) $(TARGET)
