#!/bin/sh


DEFAULT_VERSION="v1.2.0"

if test -d .git -o -f .git
then
  # (no tags, e.g. a shallow clone in CI: the default version with the commit, no error)
  GIT_VN=$(git describe --tags 2> /dev/null || true)
  if echo "$GIT_VN" | grep -Eq "^v[0-9].*"
  then
      VN="$GIT_VN"
  else
      VN="$DEFAULT_VERSION"-none-g"$(git rev-parse --short HEAD 2> /dev/null)"
  fi
else
    VN="$DEFAULT_VERSION"
fi

echo $VN


