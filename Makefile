# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/fuadshah/Desktop/MVCCART

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/fuadshah/Desktop/MVCCART

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/Applications/CLion.app/Contents/bin/cmake/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/Applications/CLion.app/Contents/bin/cmake/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/fuadshah/Desktop/MVCCART/CMakeFiles /Users/fuadshah/Desktop/MVCCART/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/fuadshah/Desktop/MVCCART/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named mvccart

# Build rule for target.
mvccart: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 mvccart
.PHONY : mvccart

# fast build rule for target.
mvccart/fast:
	$(MAKE) -f CMakeFiles/mvccart.dir/build.make CMakeFiles/mvccart.dir/build
.PHONY : mvccart/fast

#=============================================================================
# Target rules for targets named fmt

# Build rule for target.
fmt: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 fmt
.PHONY : fmt

# fast build rule for target.
fmt/fast:
	$(MAKE) -f fmt-master/fmt/CMakeFiles/fmt.dir/build.make fmt-master/fmt/CMakeFiles/fmt.dir/build
.PHONY : fmt/fast

WorkloadTests/WriteOnly.o: WorkloadTests/WriteOnly.cpp.o

.PHONY : WorkloadTests/WriteOnly.o

# target to build an object file
WorkloadTests/WriteOnly.cpp.o:
	$(MAKE) -f CMakeFiles/mvccart.dir/build.make CMakeFiles/mvccart.dir/WorkloadTests/WriteOnly.cpp.o
.PHONY : WorkloadTests/WriteOnly.cpp.o

WorkloadTests/WriteOnly.i: WorkloadTests/WriteOnly.cpp.i

.PHONY : WorkloadTests/WriteOnly.i

# target to preprocess a source file
WorkloadTests/WriteOnly.cpp.i:
	$(MAKE) -f CMakeFiles/mvccart.dir/build.make CMakeFiles/mvccart.dir/WorkloadTests/WriteOnly.cpp.i
.PHONY : WorkloadTests/WriteOnly.cpp.i

WorkloadTests/WriteOnly.s: WorkloadTests/WriteOnly.cpp.s

.PHONY : WorkloadTests/WriteOnly.s

# target to generate assembly for a file
WorkloadTests/WriteOnly.cpp.s:
	$(MAKE) -f CMakeFiles/mvccart.dir/build.make CMakeFiles/mvccart.dir/WorkloadTests/WriteOnly.cpp.s
.PHONY : WorkloadTests/WriteOnly.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... mvccart"
	@echo "... fmt"
	@echo "... WorkloadTests/WriteOnly.o"
	@echo "... WorkloadTests/WriteOnly.i"
	@echo "... WorkloadTests/WriteOnly.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

