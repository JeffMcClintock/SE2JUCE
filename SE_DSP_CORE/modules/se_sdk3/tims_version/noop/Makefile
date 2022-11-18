# Copyright 2004-2005 by Tim Hockin and Jeff McClintock

TARGET = Noop.so
SRCS = NoopPlugin.cpp
OBJS = $(SRCS:.cpp=.o)

TOPDIR = ../../..
SDKDIR = $(TOPDIR)/sdk

INCLUDES += -I$(TOPDIR)/api
INCLUDES += -I$(SDKDIR)/lib/include
INCLUDES += -I$(SDKDIR)/common/include
INCLUDES += -I$(SDKDIR)/plugin/include

DEFS = -DGMPI_TRACE_PREFIX="\"TRACE HOST\""

LDLIBS += -L$(SDKDIR)/plugin -lgmpi_plugin_sdk
LDLIBS += -L$(SDKDIR)/lib -lgmpi

CXXFLAGS = -Wall -Werror -fPIC $(INCLUDES) $(DEFS) $(_DEBUG)
LDFLAGS = -shared

ifneq ($(strip $(DEBUG)),)
_DEBUG = -ggdb
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(OBJS) $(TARGET)

