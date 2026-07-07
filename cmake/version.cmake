# generate version_gen.h from a module version file + git state. run at build
# time via `cmake -P`; configure_file only rewrites the output when the content
# actually changes, so a clean tree at the same commit causes no rebuild churn.
#
# args (all -D on the cmake -P command line):
#   VERSION_FILE  a module version.yml (newest release entry first) OR a plain
#                 file holding just the semver
#   TEMPLATE      path to version.h.in
#   OUTPUT        path to write version_gen.h
#   COMPONENT     component name baked into the banner (bootloader / APM)
#   SRC_DIR       any path inside the git repo (git -C finds the root)

if(EXISTS "${VERSION_FILE}")
  file(READ "${VERSION_FILE}" _raw)
  if(_raw MATCHES "version:")
    # version.yml: take the first (newest) "version:" entry as the base semver
    file(STRINGS "${VERSION_FILE}" _vlines REGEX "version:")
    list(GET _vlines 0 _vline)
    string(REGEX REPLACE "^[ \t-]*version:[ \t]*[\"']?([^\"' \t]+).*" "\\1"
           BASE_VERSION "${_vline}")
  else()
    string(STRIP "${_raw}" BASE_VERSION)
  endif()
else()
  set(BASE_VERSION "0.0.0")
endif()

set(FW_GIT_HASH "nogit")
set(FW_GIT_BRANCH "?")
set(FW_DIRTY 0)
set(_dirty_sfx "")

execute_process(COMMAND git -C "${SRC_DIR}" rev-parse --short=8 HEAD
                OUTPUT_VARIABLE _hash OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _r)
if(_r EQUAL 0 AND NOT _hash STREQUAL "")
  set(FW_GIT_HASH "${_hash}")

  execute_process(COMMAND git -C "${SRC_DIR}" rev-parse --abbrev-ref HEAD
                  OUTPUT_VARIABLE _br OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT _br STREQUAL "")
    set(FW_GIT_BRANCH "${_br}")
  endif()

  # ignore submodules so their state (import/ChibiOS etc.) doesn't force -dirty;
  # dirty then reflects only our own tracked changes
  execute_process(COMMAND git -C "${SRC_DIR}" status --porcelain --ignore-submodules=all
                  OUTPUT_VARIABLE _st OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT _st STREQUAL "")
    set(FW_DIRTY 1)
    set(_dirty_sfx "-dirty")
  endif()
endif()

set(FW_COMPONENT "${COMPONENT}")
set(FW_VERSION "${BASE_VERSION}")
set(FW_VERSION_STRING "${COMPONENT} v${BASE_VERSION}-${FW_GIT_HASH}${_dirty_sfx}")

configure_file("${TEMPLATE}" "${OUTPUT}" @ONLY)
