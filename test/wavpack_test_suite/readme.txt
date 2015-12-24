
                         ------------------
                         WAVPACK TEST SUITE
                         ------------------

This is intended to be a comprehensive test suite for verification
of WavPack decoders. Depending on the application and implementation
considerations, not all decoders need to be able to handle all cases.

This suite will be expanded in the future to include legacy files
(WavPack versions 1-3), multichannel files, files corrupted in various
ways to test decoders' error handling, and files that test certain
decoder options that are defined in the format but not currently
available in any public encoder (i.e. VBR lossy).

Changelog:

2006-09-04: Initial release
