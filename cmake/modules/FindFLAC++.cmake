# - Try to find Flac, the Free Lossless Audio Codec
# Once done this will define
#
#  FLAC_FOUND - system has Flac
#  FLAC_INCLUDE_DIR - the Flac include directory
#  FLAC_LIBRARIES - Link these to use Flac
#  FLAC_OGGFLAC_LIBRARIES - Link these to use OggFlac
#
# No version checking is done - use FLAC_API_VERSION_CURRENT to
# conditionally compile version-dependent code

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

FIND_PATH(FLAC++_INCLUDE_DIR FLAC++/metadata.h)

FIND_LIBRARY(FLAC++_LIBRARIES NAMES FLAC++ )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Flac++  REQUIRED_VARS  FLAC++_LIBRARIES FLAC++_INCLUDE_DIR)

# show the FLAC_INCLUDE_DIR and FLAC_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(FLAC++_INCLUDE_DIR FLAC++_LIBRARIES)
