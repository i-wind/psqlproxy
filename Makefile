CC := g++

SRCDIR := . src
OBJDIR := .build

SRCS := $(addsuffix /*.cpp, $(SRCDIR))
# SRCS := tcpproxy_server.cpp
OBJS := $(notdir $(patsubst %.cpp,%.o, $(wildcard $(SRCS))))
OBJS := $(patsubst %.o, $(OBJDIR)/%.o, $(OBJS))
CPPFLAGS += -m64 -pipe -O3 -g -Wall -W -std=c++11
LDFLAGS += -m64 -Wl,-O3 -Wl,-rpath,/usr/local/lib -g -lpthread -lboost_system \
           -lboost_program_options -lboost_thread -lboost_filesystem
APP  := sqlproxy

VPATH := $(SRCDIR)

all: build $(APP)

$(APP): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	$(CC) -c $(addprefix -I, $(SRCDIR)) $(CPPFLAGS) $< -o $@

build:
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)

strip:
	strip $(APP)

.PHONY: build strip clean
clean:
	rm -f $(OBJDIR)/*.o $(APP)
