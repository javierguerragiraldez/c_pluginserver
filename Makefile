SOURCES = main.c req_dispatch.c \
	mpack/prot.c cwpack/cwpack.c \
	plugins/plugin.c

CFLAGS = -Wall -Werror -Wextra -MMD -std=gnu11 -I $(SRC_DIR)
CXXFLAGS = -Wall -Werror -Wextra -MMD -std=c++17 -I $(SRC_DIR)
LDLIBS = -ldill




BUILD_DIR ?= build
SRC_DIR ?= src
SUBDIRS = $(sort $(filter-out ./,$(dir $(SOURCES))))

ifdef DEBUG
  CFLAGS += -g -DDEBUG
  CXXFLAGS += -g -DDEBUG
else
  CFLAGS += -O3
  CXXFLAGS += -O3
endif


PROJECT ?= $(notdir $(PWD))
APP_BIN = $(BUILD_DIR)/$(PROJECT)
APP_SOURCES = $(addprefix $(SRC_DIR)/,$(SOURCES))
APP_OBJS = $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(SOURCES))))
DEPS = $(APP_OBJS:%.o=%.d)

FRMT = "%6s %s\n"

ifndef V
  CC = @printf $(FRMT) gcc $(patsubst $(SRC_DIR)/%,%,$<) ; gcc
  CXX = @printf $(FRMT) g++ $(patsubst $(SRC_DIR)/%,%,$<) ; g++
  LN = @printf $(FRMT) link "$(patsubst $(BUILD_DIR)/%,%,$@) < $(patsubst $(BUILD_DIR)/%,%,$^)" ; gcc
else
  CC = gcc
  CXX = g++
  LN = gcc
endif


all: $(APP_BIN)

clean:
	rm -rf $(BUILD_DIR)
	mkdir -p $(addprefix $(BUILD_DIR)/,$(SUBDIRS))
.PHONY: clean

test_strval: src/strval/strval.c src/strval/strval.h
	mkdir -p tests
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TARGET_ARCH) -DTEST -DDEBUG src/strval/strval.c -o tests/strval
	tests/strval

-include $(DEPS)

$(APP_BIN): $(APP_OBJS)
	$(LN) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TARGET_ARCH) -c -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TARGET_ARCH) -c -o $@ $<
