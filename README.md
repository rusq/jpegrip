# JPEG the Ripper

Purpose: extract JPEG files from arbitrary binary data, i.e. disk images.

Could be useful for forensic analysis, or data restoration.

Initially this tool was created around 2005 when I accidently quick-formatted
the drive with photographs, mainly in JPEG format.  I did not have appropriate
recovery tools back then, so I quickly had to come up with something.

This version is an improved version of that tool (original is tagged v0.0.1 in
the Git history).

## Building

Linux/MacOS:

```shell
make jpegrip
```

Windows: untested.

## Features

- Scans arbitrary size files and extracts JPEG files, filenames are generated
  sequentially, it does not restore the original file names (see [1]).
- Reads and correctly determines the file size based on the JPEG file header
  data, i.e. if the JPEG file has a thumbnail data, it will successfully detect
  it (see [2])

NOTES
1. It detects the JPEG file merely by the JPEG file signature, in order to
   detect the original name, we would have to know the container format, i.e. if
   it is a disk image, we would need to correctly parse the FAT to get the
   filename, which would make the tool quite complex.
2. The initial version was searching for the JPEG SOI and EOI tags (see [JPEG
   tags][1]), and extract everything in between.  The problem with this approach
   was that the thumbnail image, embedded in the JFIF file also ends with EOI
   tag, so the initial versions were extracting the Thumbnail with some
   malformed header, which, of course, most image viewers would not be able to
   open or view.  To address that I tried using the `libjpeg`, which would be an
   overkill and also introduced the dependency, making it harder to compile on
   different platforms.  So I did the right thing, and implemented a simple JPEG
   header parser (in [jpeg.c][2]), the only purpose of which is to find the
   beginning of the image data and return the difference in bytes between start
   and discovered address.  It does so by reading through the file and skipping
   all sections until it reaches the Quantisation Table.  After examining a
   number of JPEG files, it seems that it is guaranteed to be after all Exif and
   Comment data etc., please let me know if you know otherwise.

## License
BSD 3-clause.

[1]: https://www.digicamsoft.com/itu/itu-t81-36.html
[2]: jpeg.c
