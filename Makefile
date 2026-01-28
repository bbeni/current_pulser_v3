# setting flags 0 or 1
PACK_RESOURCES := 1

# Resources that need be packed (yes for now we need to add all here)
PACKED_RESOURCES = \
	resources/font/NimbusSans-Regular.ttf \
	resources/font/NimbusSans-Bold.ttf \
	resources/font/NimbusSans-Italic.ttf \
	resources/font/NimbusSans-BoldItalic.ttf \

# Platform detection
ifeq ($(OS),Windows_NT)
    PLATFORM := WINDOWS
	RAYLIB_PATH := src/thirdparty/raylib-5.5
	STATIC_LIBS :=
	LDFLAGS_PLATFORM := -lopengl32 -lgdi32 -lwinmm -Wl,-subsystem,console -lShlwapi
	EXT := .exe
else
    PLATFORM := LINUX
	RAYLIB_PATH := src/thirdparty/raylib-5.5-linux
	STATIC_LIBS := $(RAYLIB_PATH)/lib/libraylib.a
	LDFLAGS_PLATFORM := -lGL -lm -lpthread -ldl -lrt -lX11
	EXT :=
endif

CC := gcc
LD := gcc

# Directories
SRC_DIR := src
BUILD_DIR := build
RESOURCES_BUILD_DIR := build/res
THIRDPARTY_DIR := src/thirdparty

# binaries
RESOURCE_PACKER_BINARY := $(RESOURCES_BUILD_DIR)/packer$(EXT)
TARGET := $(BUILD_DIR)/impedancer$(EXT)

# Resources stuff
COUNT := $(words $(PACKED_RESOURCES))
RES_SEQ := $(shell seq $(COUNT))
RES_C_FILES := $(patsubst %, $(RESOURCES_BUILD_DIR)/data_%.c, $(RES_SEQ))
RES_O_FILES := $(patsubst %, $(RESOURCES_BUILD_DIR)/data_%.o, $(RES_SEQ))
RESOURCE_PACKER_C := resources/resource_packer.c
THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))

# Files
SRCS := $(SRC_DIR)/main.c \
		$(SRC_DIR)/mui.c \
		$(SRC_DIR)/gra.c \
		$(SRC_DIR)/uti.c \
		$(SRC_DIR)/mma.c \
		$(SRC_DIR)/mui_platform_raylib.c \

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
HEADER_DEPS := $(SRC_DIR)/mui.h $(SRC_DIR)/gra.h $(SRC_DIR)/uti.h $(SRC_DIR)/mma.h

CFLAGS := -Wall -Wextra -Iinclude -I$(RAYLIB_PATH)/include -I$(THIRDPARTY_DIR) -ggdb
LDFLAGS := $(STATIC_LIBS) -L$(RAYLIB_PATH)/lib -lraylib $(LDFLAGS_PLATFORM)

ifeq ($(PACK_RESOURCES), 0)
	RES_DEPENDENCY =
else
	CFLAGS += -DRESOURCE_PACKER
	CFLAGS += -I$(RESOURCES_BUILD_DIR)
	SRCS += $(RESOURCES_BUILD_DIR)/resource_accessor.c $(RES_C_FILES)
	OBJS += $(RESOURCES_BUILD_DIR)/resource_accessor.o $(RES_O_FILES)
	RES_DEPENDENCY = $(RESOURCES_BUILD_DIR)/resource_accessor.h
endif

.PHONY: all
all: $(TARGET)

.PHONY: run
run: $(TARGET)
	$(TARGET) $(ARGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(RESOURCES_BUILD_DIR):
	mkdir -p $(RESOURCES_BUILD_DIR)

$(RESOURCES_BUILD_DIR)/data_%.o: $(RESOURCES_BUILD_DIR)/data_%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(RESOURCE_PACKER_BINARY): $(RESOURCE_PACKER_C) $(SRC_DIR)/uti.c $(THIS_MAKEFILE) | $(RESOURCES_BUILD_DIR)
	@echo "Building resource packer..."
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(RESOURCE_PACKER_C) $(SRC_DIR)/uti.c -o $@ $(LDFLAGS)

$(RES_C_FILES) $(RESOURCES_BUILD_DIR)/resource_accessor.c $(RESOURCES_BUILD_DIR)/resource_accessor.h: $(RESOURCE_PACKER_BINARY) $(PACKED_RESOURCES) $(THIS_MAKEFILE)
	@echo "Running resource packer..."
	$(RESOURCE_PACKER_BINARY) $(RESOURCES_BUILD_DIR) $(PACKED_RESOURCES)
	@touch $(RESOURCES_BUILD_DIR)/resource_accessor.h # Ensures timestamp is updated

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_DEPS) $(RES_DEPENDENCY) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: tests
tests: $(SRC_DIR)/mma_tests.c $(SRC_DIR)/mma.c
	$(CC) $(CFLAGS) $(SRC_DIR)/mma_tests.c $(SRC_DIR)/mma.c -o $(BUILD_DIR)/tests $(LDFLAGS)
	$(BUILD_DIR)/tests

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*

