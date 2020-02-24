#
# Starter Makefile for WatDFS (CS 454/654).
# You may change this file.
#

# make clean all --- cleans and produces libwatdfs.a watdfs_server watdfs_client
# make zip --- cleans and produces a zip file

# Add files you want to go into your client library here.
WATDFS_CLI_FILES= watdfs_client.cpp
WATDFS_CLI_OBJS= watdfs_client.o

# Add files you want to go into your server here.
WATDFS_SERVER_FILES = watdfs_server.cpp
WATDFS_SERVER_OBJS = watdfs_server.o
# E.g. for A3 add rw_lock.c and rw_lock.o to the
# WATDFS_SERVER_FILES and WATDFS_SERVER_OBJS respectively.

CXX = g++

# Add the required fuse library includes.
CXXFLAGS += $(shell pkg-config --cflags fuse)
CXXFLAGS += -g -Wall -std=c++1y -MMD
# If you want to disable logging messages from DLOG, uncomment the next line.
#CXXFLAGS += -DNDEBUG

# Add fuse libraries.
LDFLAGS += $(shell pkg-config --libs fuse)

# Dependencies for the client executable.
WATDFS_CLIENT_LIBS = libwatdfsmain.a libwatdfs.a librpc.a

OBJECTS = $(WATDFS_SERVER_OBJS) $(WATDFS_CLI_OBJS)
DEPENDS = $(OBJECTS:.o=.d)

# targets
.DEFAULT_GOAL = default_goal

default_goal: libwatdfs.a watdfs_server

# By default make libwatdfs.a and watdfs_server.
all: libwatdfs.a watdfs_server watdfs_client

# This compiles object files, by default it looks for .c files
# so you may want to change this depending on your file naming scheme.
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(LDFLAGS) -L. -lrpc $<

# Make the client library.
libwatdfs.a: $(WATDFS_CLI_OBJS)
	ar rc $@ $^

# Make the server executable.
watdfs_server: $(WATDFS_SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -L. -lrpc -o $@

# Make the client executable.
watdfs_client: $(WATDFS_CLIENT_LIBS)
	$(CXX) $(CXXFLAGS) -o watdfs_client -L. -lwatdfsmain -lwatdfs -lrpc $(LDFLAGS)

# Add dependencies so object files are tracked in the correct order.
depend:
	makedepend -f- -- $(CXXFLAGS) -- $(WATDFS_SERVER_FILES) $(WATDFS_CLI_FILES) > .depend

-include $(DEPENDS)

# Clean up extra dependencies and objects.
clean:
	/bin/rm -f $(DEPENDS) $(OBJECTS) watdfs_server libwatdfs.a watdfs_client

zip: clean createzip

# Update as required.
createzip:
	zip -r watdfs.zip $(WATDFS_SERVER_FILES) $(WATDFS_CLI_FILES) Makefile *.h
