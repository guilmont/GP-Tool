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
include vendor/CMakeFiles/implot.dir/depend.make

# Include the progress variables for this target.
include vendor/CMakeFiles/implot.dir/progress.make

# Include the compile flags for this target's objects.
include vendor/CMakeFiles/implot.dir/flags.make

vendor/CMakeFiles/implot.dir/implot/implot.cpp.o: vendor/CMakeFiles/implot.dir/flags.make
vendor/CMakeFiles/implot.dir/implot/implot.cpp.o: ../vendor/implot/implot.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gstark/Dropbox/gp_fbm_library/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object vendor/CMakeFiles/implot.dir/implot/implot.cpp.o"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/implot.dir/implot/implot.cpp.o -c /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot.cpp

vendor/CMakeFiles/implot.dir/implot/implot.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/implot.dir/implot/implot.cpp.i"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot.cpp > CMakeFiles/implot.dir/implot/implot.cpp.i

vendor/CMakeFiles/implot.dir/implot/implot.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/implot.dir/implot/implot.cpp.s"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot.cpp -o CMakeFiles/implot.dir/implot/implot.cpp.s

vendor/CMakeFiles/implot.dir/implot/implot_demo.cpp.o: vendor/CMakeFiles/implot.dir/flags.make
vendor/CMakeFiles/implot.dir/implot/implot_demo.cpp.o: ../vendor/implot/implot_demo.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gstark/Dropbox/gp_fbm_library/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object vendor/CMakeFiles/implot.dir/implot/implot_demo.cpp.o"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/implot.dir/implot/implot_demo.cpp.o -c /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot_demo.cpp

vendor/CMakeFiles/implot.dir/implot/implot_demo.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/implot.dir/implot/implot_demo.cpp.i"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot_demo.cpp > CMakeFiles/implot.dir/implot/implot_demo.cpp.i

vendor/CMakeFiles/implot.dir/implot/implot_demo.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/implot.dir/implot/implot_demo.cpp.s"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot_demo.cpp -o CMakeFiles/implot.dir/implot/implot_demo.cpp.s

vendor/CMakeFiles/implot.dir/implot/implot_items.cpp.o: vendor/CMakeFiles/implot.dir/flags.make
vendor/CMakeFiles/implot.dir/implot/implot_items.cpp.o: ../vendor/implot/implot_items.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gstark/Dropbox/gp_fbm_library/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object vendor/CMakeFiles/implot.dir/implot/implot_items.cpp.o"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/implot.dir/implot/implot_items.cpp.o -c /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot_items.cpp

vendor/CMakeFiles/implot.dir/implot/implot_items.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/implot.dir/implot/implot_items.cpp.i"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot_items.cpp > CMakeFiles/implot.dir/implot/implot_items.cpp.i

vendor/CMakeFiles/implot.dir/implot/implot_items.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/implot.dir/implot/implot_items.cpp.s"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gstark/Dropbox/gp_fbm_library/vendor/implot/implot_items.cpp -o CMakeFiles/implot.dir/implot/implot_items.cpp.s

# Object files for target implot
implot_OBJECTS = \
"CMakeFiles/implot.dir/implot/implot.cpp.o" \
"CMakeFiles/implot.dir/implot/implot_demo.cpp.o" \
"CMakeFiles/implot.dir/implot/implot_items.cpp.o"

# External object files for target implot
implot_EXTERNAL_OBJECTS =

vendor/libimplot.a: vendor/CMakeFiles/implot.dir/implot/implot.cpp.o
vendor/libimplot.a: vendor/CMakeFiles/implot.dir/implot/implot_demo.cpp.o
vendor/libimplot.a: vendor/CMakeFiles/implot.dir/implot/implot_items.cpp.o
vendor/libimplot.a: vendor/CMakeFiles/implot.dir/build.make
vendor/libimplot.a: vendor/CMakeFiles/implot.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gstark/Dropbox/gp_fbm_library/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX static library libimplot.a"
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && $(CMAKE_COMMAND) -P CMakeFiles/implot.dir/cmake_clean_target.cmake
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/implot.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
vendor/CMakeFiles/implot.dir/build: vendor/libimplot.a

.PHONY : vendor/CMakeFiles/implot.dir/build

vendor/CMakeFiles/implot.dir/clean:
	cd /home/gstark/Dropbox/gp_fbm_library/build/vendor && $(CMAKE_COMMAND) -P CMakeFiles/implot.dir/cmake_clean.cmake
.PHONY : vendor/CMakeFiles/implot.dir/clean

vendor/CMakeFiles/implot.dir/depend:
	cd /home/gstark/Dropbox/gp_fbm_library/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gstark/Dropbox/gp_fbm_library /home/gstark/Dropbox/gp_fbm_library/vendor /home/gstark/Dropbox/gp_fbm_library/build /home/gstark/Dropbox/gp_fbm_library/build/vendor /home/gstark/Dropbox/gp_fbm_library/build/vendor/CMakeFiles/implot.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : vendor/CMakeFiles/implot.dir/depend

