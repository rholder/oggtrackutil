What is oggtrackutil?
========
This is a simple audio extraction utility written during a weekend to extract well-formed, single track WAV files from any compressed Ogg Vorbis audio file including those with more than 8 tracks.

Multitrack Ogg Vorbis audio files with more than 8 tracks seem to give FFmpeg a fit.  Audacity doesn't seem to have a problem with opening them and letting you peak inside at their guts, but it's so much trouble to have to click around in a GUI to extract every channel as a separate single track WAV.  So, I picked apart the good bits of the algorithm Audacity uses for processing these things and rolled them into this little command line utility.  It should be able to handle a good set of multichannel audio types inside of an Ogg Vorbis container since it uses libvorbis to extract the WAV audio and then write it back out to individual files, preserving the sample rate of the original source.

While it's not the cleanest or most efficient bit of source code in the world, it might serve as a reasonable complete example of utilizing libvorbis directly in C.

How do I build this cool thing?
========

First, you'll want to make sure you have CMake and the libvorbis development headers. If you're using some flavor of Ubuntu, then you can install them with:

`sudo apt-get install cmake libvorbis-dev`

Next, you'll want CMake to generate some standard Makefile goodies specific for your system:

`cmake -G 'Unix Makefiles'`

Now you can just build it by running:

`make`

If all goes well, then it should be ready to use now.

How do I use it?
========

Just pass it a multitrack Ogg audio file and watch it go.

`oggtrackutil myawesomesong.ogg`
