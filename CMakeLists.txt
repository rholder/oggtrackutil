# To create a standard Makefile, run:
#     cmake -G 'Unix Makefiles'
cmake_minimum_required (VERSION 2.6)
project (oggtrackutil)

# Add executable called "oggtrackutil" that is built from the source file
# "oggtrackutil.c". The extensions are automatically found.
add_executable (oggtrackutil oggtrackutil.c)

# Link the executable to the vorbisfile library.
target_link_libraries (oggtrackutil vorbisfile)
