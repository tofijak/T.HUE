#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := T.HUE

include $(IDF_PATH)/make/project.mk

CFLAGS=-w 
CFLAGS += -std=c++14
