# Pinned downloads, with a local mirror for building without a network.
#
# dawit_pinned(<NAME> <url> <sha256>) sets <NAME>_URL and <NAME>_HASH for FetchContent_Declare (URL, URL_HASH) or
# file(DOWNLOAD) (EXPECTED_HASH). When the file is in the mirror, <NAME>_URL points to it there; otherwise it is the
# upstream URL. Either way the hash is checked, so a mirrored file can't differ from the upstream one.
#
# The mirror keeps each file at its URL without "https://": third_party/mirror/github.com/sevagh/demucs.cpp/...
# ONNX Runtime's own build reads the same layout (--cmake_deps_mirror_dir), so one folder serves both. The complete
# source archive fills it (tools/make-complete-source.py); a git checkout has none and downloads as usual.
#
# tools/make-complete-source.py reads every dawit_pinned call in CMakeLists.txt, so keep each on one line with a
# literal URL (${VAR} only for a variable set with a literal value) and a literal hash.

set(DAWIT_MIRROR_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/mirror" CACHE PATH
    "Local mirror of pinned downloads, laid out by URL (filled by the complete source archive)")
get_filename_component(DAWIT_MIRROR_DIR "${DAWIT_MIRROR_DIR}" ABSOLUTE)

function(dawit_pinned name url sha256)
    string(REGEX REPLACE "^https://" "" relative "${url}")
    set(local "${DAWIT_MIRROR_DIR}/${relative}")

    if(EXISTS "${local}")
        # A file:// URL works for both FetchContent and file(DOWNLOAD); Windows paths need the third slash.
        if(local MATCHES "^/")
            set(${name}_URL "file://${local}" PARENT_SCOPE)
        else()
            set(${name}_URL "file:///${local}" PARENT_SCOPE)
        endif()
    else()
        set(${name}_URL "${url}" PARENT_SCOPE)
    endif()

    set(${name}_HASH "SHA256=${sha256}" PARENT_SCOPE)
endfunction()
