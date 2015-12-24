
                        ----------------------
                        WAVPACK BITDEPTHS TEST
                        ----------------------

The files in this folder test all the possible bitdepths that the WavPack
format allows. All files contain md5 sums, so they can be easily verified.

For the 32-bit integer and float files, there is also a version encoded
with the -p command-line option. This limits the resolution for any given
WavPack block to 25 bits (although the dynamic range of float files is
maintained from block to block). Because this is technically a lossy
process, the md5 sums will not match for these files, however the actual
difference between the versions is never more than 140 dB below fullscale
(i.e. probably not measurable, and certainly not audible).

The 30-second audio clip is the same for all the files, however the
conversion to the different source bitdepths (from a 24/96 original) was
done by CoolEdit2000, except for the 12-bit version which CoolEdit cannot
handle.

