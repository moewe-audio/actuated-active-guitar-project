# Project Name
TARGET = AcousticGuitarController

# Library Locations
LIBDAISY_DIR ?= lib/libDaisy
DAISYSP_DIR ?= lib/DaisySP

OPT = -Os

# application sources
CPP_SOURCES = main_pd.cpp \
              max98389.cpp

# Core Daisy makefile
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
