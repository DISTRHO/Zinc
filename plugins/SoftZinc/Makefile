#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = SoftZinc

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	DistrhoPluginSoftZinc.cpp

# --------------------------------------------------------------
# Do some magic

include ../../dpf/Makefile.plugins.mk

BUILD_CXX_FLAGS += -pthread
LINK_FLAGS      += -pthread

# --------------------------------------------------------------
# Enable all possible plugin types

all: clap dssi_dsp lv2_dsp vst2 vst3

# --------------------------------------------------------------
