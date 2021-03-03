# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.19

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/gstark/Dropbox/gp_fbm_library

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/gstark/Dropbox/gp_fbm_library/build

# Include any dependencies generated for this target.
include vendor/CMakeFiles/glad.dir/depend.make

# Include the progress variables for this target.
include vendor/CMakeFiles/glad.dir/progress.make

# Include the compile flags for this target's objects.
include vendor/CMakeFiles/glad.dir/flags.make

vendor/CMakeFiles/glad.dir/glad/src/glad.c.o: vendor/CMakeFiles/glad.dir/flags.make
vendor/CMakeFiles/glad.dir/glad/src/glad.c.o: ../vendor/glad/src/glad.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gstark/Dropbox/gp_fbm_library/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object vendor/CMakeFiles/glad.dir/glad/src/glad.c.o"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/glad.dir/glad/src/glad.c.o -c /home/gstark/Dropbox/gp_fbm_library/vendor/glad/src/glad.c

vendor/CMakeFiles/glad.dir/glad/src/glad.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/glad.dir/glad/src/glad.c.i"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/gstark/Dropbox/gp_fbm_library/vendor/glad/src/glad.c > CMakeFiles/glad.dir/glad/src/glad.c.i

vendor/CMakeFiles/glad.dir/glad/src/glad.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/glad.dir/glad/src/glad.c.s"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/gstark/Dropbox/gp_fbm_library/vendor/glad/src/glad.c -o CMakeFiles/glad.dir/glad/src/glad.c.s

# Object files for target glad
glad_OBJECTS = \
"CMakeFiles/glad.dir/glad/src/glad.c.o"

# External object files for target glad
glad_EXTERNAL_OBJECTS =

vendor/libglad.a: vendor/CMakeFiles/glad.dir/glad/src/glad.c.o
vendor/libglad.a: vendor/CMakeFiles/glad.dir/build.make
vendor/libglad.a: vendor/CMakeFiles/glad.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gstark/Dropbox/gp_fbm_library/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libglad.a"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && $(CMAKE_COMMAND) -P CMakeFiles/glad.dir/cmake_clean_target.cmake
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/glad.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
vendor/CMakeFiles/glad.dir/build: vendor/libglad.a

.PHONY : vendor/CMakeFiles/glad.dir/build

vendor/CMakeFiles/glad.dir/clean:
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && $(CMAKE_COMMAND) -P CMakeFiles/glad.dir/cmake_clean.cmake
.PHONY : vendor/CMakeFiles/glad.dir/clean

vendor/CMakeFiles/glad.dir/depend:
	cd /home/gstark/Dropbox/gp_fbm_library/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gstark/Dropbox/gp_fbm_library /home/gstark/Dropbox/gp_fbm_library/vendor /home/gstark/Dropbox/gp_fbm_library/build /home/gstark/Dropbox/gp_fbm_library/build/vendor /home/gstark/Dropbox/gp_fbm_library/build/vendor/CMakeFiles/glad.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : vendor/CMakeFiles/glad.dir/depend

