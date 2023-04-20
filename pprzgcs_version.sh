#!/bin/sh


DEFAULT_VERSION="v1.0.9"

if test -d .git -o -f .git
then
  GIT_VN=$(git describe --tags)
  if echo "$GIT_VN" | grep -Eq "^v[0-9].*"
  then
      VN="$GIT_VN"
  else
      VN="$DEFAULT_VERSION"-none-g"$GIT_VN"
  fi
else
    VN="$DEFAULT_VERSION"
fi

echo $VN


