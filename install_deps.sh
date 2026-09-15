#!/usr/bin/env bash
# install_deps.sh - install the Ubuntu packages needed to build and package PprzGCS
#
#   ./install_deps.sh [--check] [--dry-run] [--yes] [GROUP...]
#
# Groups (default: build; appimage and deb include build):
#   build     compile and run PprzGCS: ./build.sh
#   appimage  package it as an AppImage: ./make_appimage.sh
#   deb       package it as a .deb: dpkg-buildpackage
#   all       every group
#
#   --check    print the missing packages (nothing when all are installed), exit 1 when some are
#              missing; installs nothing
#   --dry-run  print the commands without running them
#   --yes      install without asking (needed outside a terminal)
#
# Tested on Ubuntu 22.04 and 24.04. "a|b" is a package with another name on some releases: the first
# one apt knows is installed (e.g. Qt6 SVG is libqt6svg6-dev on 22.04 and qt6-svg-dev on 24.04).
set -euo pipefail

declare -A GROUP_PACKAGES=(
  [build]="build-essential cmake extra-cmake-modules qt6-base-dev libqt6svg6-dev|qt6-svg-dev qt6-qpa-plugins libgl-dev libglu1-mesa-dev mesa-common-dev libproj-dev libzip-dev libxml2-dev libsqlite3-dev"
  [appimage]="qt6-base-dev-tools qmake6 file wget ca-certificates"
  [deb]="dpkg-dev debhelper fakeroot git"
)
GROUPS_ORDER=(build appimage deb)
TESTED_RELEASES="jammy noble"

usage() { sed -n '3,18p' "$0" | sed 's/^# \{0,1\}//'; }
die() { printf 'install_deps.sh: %s\n' "$*" >&2; exit 2; }
run() { printf '+ %s\n' "$*"; "$@"; }

CHECK=false
DRY_RUN=false
YES=false
groups=()
while [ $# -gt 0 ]; do
  case "$1" in
    --check) CHECK=true ;;
    --dry-run) DRY_RUN=true ;;
    -y|--yes) YES=true ;;
    -h|--help) usage; exit 0 ;;
    all) groups+=("${GROUPS_ORDER[@]}") ;;
    -*) usage >&2; die "unknown option: $1" ;;
    *) [ -n "${GROUP_PACKAGES[$1]+set}" ] || die "unknown group '$1' (${GROUPS_ORDER[*]}, or all)"
       groups+=("$1") ;;
  esac
  shift
done
[ "${#groups[@]}" -gt 0 ] || groups=(build)

# the packages of the groups, build always included, without duplicates
packages=()
for group in build "${groups[@]}"; do
  for package in ${GROUP_PACKAGES[$group]}; do
    case " ${packages[*]} " in *" $package "*) ;; *) packages+=("$package") ;; esac
  done
done

if ! command -v dpkg-query > /dev/null || ! command -v apt-get > /dev/null; then
  echo "install_deps.sh: not a Debian/Ubuntu system, install the equivalents of:" \
       "${packages[*]//|/ or }" >&2
  $CHECK && exit 0  # nothing to check here: do not block a build
  exit 1
fi
# shellcheck source=/dev/null
codename="$(. /etc/os-release 2> /dev/null && echo "${VERSION_CODENAME:-}")"
case " $TESTED_RELEASES " in
  *" $codename "*) ;;
  *) echo "install_deps.sh: tested on Ubuntu 22.04 and 24.04; on ${codename:-this system}" \
          "package names may differ" >&2 ;;
esac

# the installed packages among every name (dpkg-query fails when some name is unknown: its output
# still lists the others)
# shellcheck disable=SC2206
names=(${packages[*]//|/ })
installed="$( (dpkg-query -W -f='${Package} ${db:Status-Abbrev}\n' "${names[@]}" 2> /dev/null || true) |
  awk '$2 ~ /^ii/ {print $1}')"
missing=()  # the packages none of whose names is installed
for package in "${packages[@]}"; do
  found=false
  for name in ${package//|/ }; do
    if grep -qx "$name" <<< "$installed"; then found=true; fi
  done
  $found || missing+=("$package")
done

# the first name of a|b apt can install ("" when none)
candidate() {
  local name version
  for name in ${1//|/ }; do
    version="$(apt-cache policy "$name" 2> /dev/null | sed -n 's/^  Candidate: //p')"
    if [ -n "$version" ] && [ "$version" != "(none)" ]; then
      echo "$name"
      return
    fi
  done
}
# the missing packages by the names apt knows (as written when it knows none yet)
missing_names() {
  local package name
  for package in "${missing[@]}"; do
    name="$(candidate "$package")"
    printf '%s ' "${name:-$package}"
  done
}

if $CHECK; then
  [ "${#missing[@]}" -eq 0 ] && exit 0
  missing_names | sed 's/ $/\n/'
  exit 1
fi
if [ "${#missing[@]}" -eq 0 ]; then
  echo "the packages of ${groups[*]} are installed"
  exit 0
fi

sudo=()
[ "$(id -u)" -eq 0 ] || sudo=(sudo)
echo "missing packages (${groups[*]}): $(missing_names)"
if $DRY_RUN; then
  echo "+ ${sudo[*]} apt-get update"
  echo "+ ${sudo[*]} apt-get install -y --no-install-recommends $(missing_names)"
  exit 0
fi
if ! $YES; then
  [ -t 0 ] || die "not a terminal: add --yes to install without asking"
  read -r -p "Install them now? [Y/n] " answer
  case "$answer" in
    ""|[yY]*) ;;
    *) echo "nothing installed"; exit 1 ;;
  esac
fi
run "${sudo[@]}" apt-get update
to_install=()
for package in "${missing[@]}"; do
  name="$(candidate "$package")"
  [ -n "$name" ] || die "no package ${package//|/ or } in this system's apt repositories"
  to_install+=("$name")
done
run "${sudo[@]}" apt-get install -y --no-install-recommends "${to_install[@]}"
