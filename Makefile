# setting flags 0 or 1
PACK_RESOURCES := 1

# Resources that need be packed
PACKED_RESOURCES = \
    resources/font/NimbusSans-Regular.ttf \
    resources/font/NimbusSans-Bold.ttf \
    resources/font/NimbusSans-Italic.ttf \
    resources/font/NimbusSans-BoldItalic.ttf

CC := gcc
LD := gcc

# Directories
SRC_DIR := src
BUILD_DIR := build
RESOURCES_BUILD_DIR := build/res
THIRDPARTY_DIR := src/thirdparty

# Platform detection
ifeq ($(OS),Windows_NT)
    PLATFORM := WINDOWS
    RAYLIB_PATH := $(THIRDPARTY_DIR)/raylib-5.5
    WAVEFORMS_SDK_PATH := "C:\Program Files (x86)\Digilent\WaveFormsSDK"
    LDFLAGS_PLATFORM := -lopengl32 -lgdi32 -lwinmm -Wl,-subsystem,console -lShlwapi
    EXT := .exe
else
    PLATFORM := LINUX
    RAYLIB_PATH := $(THIRDPARTY_DIR)/raylib-5.5-linux
    STATIC_LIBS := $(RAYLIB_PATH)/lib/libraylib.a
    LDFLAGS_PLATFORM := -lGL -lm -lpthread -ldl -lrt -lX11
    EXT :=
endif

# binaries
RESOURCE_PACKER_BINARY := $(RESOURCES_BUILD_DIR)/packer$(EXT)
TARGET := $(BUILD_DIR)/current_pulser$(EXT)
TARGET_OSCILLOSCOPE := $(BUILD_DIR)/oscilloscope$(EXT)

# Resources stuff
COUNT := $(words $(PACKED_RESOURCES))
RES_SEQ := $(shell seq $(COUNT))
RES_C_FILES := $(patsubst %, $(RESOURCES_BUILD_DIR)/data_%.c, $(RES_SEQ))
RES_O_FILES := $(patsubst %, $(RESOURCES_BUILD_DIR)/data_%.o, $(RES_SEQ))
RESOURCE_PACKER_C := resources/resource_packer.c

# Files
COMMON_SRCS := $(SRC_DIR)/mui.c \
            $(SRC_DIR)/gra.c \
            $(SRC_DIR)/uti.c \
            $(SRC_DIR)/mma.c \
            $(SRC_DIR)/osci_control.c \
            $(SRC_DIR)/mui_platform_raylib.c

OBJS_MAIN := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_DIR)/main.c $(COMMON_SRCS))
OBJS_OSCI := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_DIR)/main_osci.c $(COMMON_SRCS))

HEADER_DEPS := $(SRC_DIR)/mui.h $(SRC_DIR)/gra.h $(SRC_DIR)/uti.h $(SRC_DIR)/mma.h
CFLAGS := -Wall -Wextra -Iinclude -I$(RAYLIB_PATH)/include -I$(THIRDPARTY_DIR) -I$(WAVEFORMS_SDK_PATH)/inc -ggdb
LDFLAGS := $(STATIC_LIBS) -L$(RAYLIB_PATH)/lib -L$(WAVEFORMS_SDK_PATH)/lib/x64 -ldwf -lraylib  $(LDFLAGS_PLATFORM) -ggdb

ifeq ($(PACK_RESOURCES), 1)
    CFLAGS += -DRESOURCE_PACKER -I$(RESOURCES_BUILD_DIR)
    OBJS_MAIN += $(RESOURCES_BUILD_DIR)/resource_accessor.o $(RES_O_FILES)
    OBJS_OSCI += $(RESOURCES_BUILD_DIR)/resource_accessor.o $(RES_O_FILES)
    RES_DEPENDENCY = $(RESOURCES_BUILD_DIR)/resource_accessor.h
endif

.PHONY: all
all: $(TARGET) $(TARGET_OSCILLOSCOPE)

.PHONY: run
run: $(TARGET)
	$(TARGET) $(ARGS)

.PHONY: run_osci
run_osci: $(TARGET_OSCILLOSCOPE)
	$(TARGET_OSCILLOSCOPE) $(ARGS)

$(BUILD_DIR) $(RESOURCES_BUILD_DIR):
	mkdir -p $@

$(RESOURCES_BUILD_DIR)/data_%.o: $(RESOURCES_BUILD_DIR)/data_%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(RESOURCE_PACKER_BINARY): $(RESOURCE_PACKER_C) $(SRC_DIR)/uti.c | $(RESOURCES_BUILD_DIR)
	@echo "Building resource packer..."
	$(CC) $(CFLAGS) -I$(SRC_DIR) $^ -o $@ $(LDFLAGS)

$(RES_C_FILES) $(RESOURCES_BUILD_DIR)/resource_accessor.c $(RESOURCES_BUILD_DIR)/resource_accessor.h: $(RESOURCE_PACKER_BINARY) $(PACKED_RESOURCES)
	@echo "Running resource packer..."
	$(RESOURCE_PACKER_BINARY) $(RESOURCES_BUILD_DIR) $(PACKED_RESOURCES)
	@touch $(RESOURCES_BUILD_DIR)/resource_accessor.h

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_DEPS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o $(BUILD_DIR)/main_osci.o $(BUILD_DIR)/mui_platform_raylib.o: $(RES_DEPENDENCY)

$(TARGET): $(OBJS_MAIN)
	$(LD) $(OBJS_MAIN) -o $@ $(LDFLAGS)

$(TARGET_OSCILLOSCOPE): $(OBJS_OSCI)
	$(LD) $(OBJS_OSCI) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*