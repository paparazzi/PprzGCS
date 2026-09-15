#!/usr/bin/env bash
# build.sh - build PprzGCS from sources, into build/ (the program: build/pprzgcs)
#
#   ./build.sh [--skip-checks] [--debug] [-DCMAKE_OPTION=VALUE...]      e.g. ./build.sh -DGRPC=ON
#
# Before building (not with --skip-checks):
#   - the packages of `./install_deps.sh build`: when some are missing it offers to install them
#     (in a terminal), otherwise it prints the command and stops. Not checked with a Qt that is not
#     the system's (CMAKE_PREFIX_PATH set, or build/ already configured with another Qt)
#   - the submodules (pprzlinkQt, and IvyQt in it): fetched when missing
#
# Environment: WD (build in $WD/build; default: the repository), install_prefix (CMake install
# prefix; default: $WD/build/install), MAKEFLAGS (default: -j<number of cores>).
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
REPO="$(pwd)"
WD="${WD:-$REPO}"
install_prefix="${install_prefix:-$WD/build/install}"
export MAKEFLAGS="${MAKEFLAGS:--j$(nproc 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null || echo 2)}"

CHECKS=true
BUILD_TYPE=RelWithDebInfo
cmake_options=()
while [ $# -gt 0 ]; do
  case "$1" in
    --skip-checks) CHECKS=false ;;
    --debug) BUILD_TYPE=Debug ;;
    -h|--help) sed -n '3,13p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    -D*) cmake_options+=("$1") ;;
    *) echo "build.sh: unknown argument '$1' (./build.sh --help)" >&2; exit 2 ;;
  esac
  shift
done

# a Qt that is not the system's (e.g. from the Qt installer): the system packages do not apply
own_qt="${CMAKE_PREFIX_PATH:-}"
own_qt="${own_qt%:}"
if [ -z "$own_qt" ]; then
  own_qt="$(sed -n 's|^Qt6_DIR:PATH=||p' "$WD/build/CMakeCache.txt" 2> /dev/null | grep -v '^/usr/' || true)"
fi

if $CHECKS; then
  if [ -n "$own_qt" ]; then
    echo "Qt from $own_qt: packages not checked (./install_deps.sh --check lists them)"
  elif ! missing="$(./install_deps.sh --check build)"; then
    echo "PprzGCS needs packages that are not installed: $missing"
    if [ -t 0 ] && [ -t 1 ]; then
      ./install_deps.sh build  # asks before installing
    else
      echo "install them with ./install_deps.sh build (or build anyway: ./build.sh --skip-checks)" >&2
      exit 1
    fi
  fi
  if [ ! -f ext/pprzlinkQt/CMakeLists.txt ] || [ ! -f ext/pprzlinkQt/IvyQt/CMakeLists.txt ]; then
    echo "fetching the submodules pprzlinkQt and IvyQt"
    git submodule update --init --recursive || {
      echo "build.sh: cannot fetch the submodules (is this a git clone, with access to github.com" \
           "and gitlab.com?)" >&2
      exit 1
    }
  fi
fi

echo "building PprzGCS $(./pprzgcs_version.sh) ($BUILD_TYPE) in $WD/build"
cmake -S "$REPO" -B "$WD/build" -DCMAKE_INSTALL_PREFIX="$install_prefix" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "${cmake_options[@]}"
cmake --build "$WD/build"

cat << EOF

built: $WD/build/pprzgcs
- from a terminal: set PAPARAZZI_HOME and PAPARAZZI_SRC to your Paparazzi, then run $WD/build/pprzgcs
- from the Paparazzi Center: add $WD/build to your PATH
EOF
