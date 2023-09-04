
How to make a new release
=========================

1. Update `debian/changelog` with: `$ dch`. Makes sure the author name and email matches one of your gpg keys (get the list with `gpg --list-secret-keys`).
2. Update `pprzgcs_version.sh` with the correct DEFAULT_VERSION. (TODO: use version from debian/changelog)
3. Commit your changes and tag the commit.
4. `git ls-files --recurse-submodules | tar -cvf pprzgcs.tar -T -`
5. `mkdir ../pprzgcs_release && src=$(pwd) && cd ../pprzgcs_release`
6. `tar -xvf "$src/pprzgcs.tar"`
7. Install dependencies: `sudo apt install ivy-qt-dev`
8. `debuild -S`
9. `dput ppa:paparazzi-uav/ppa <pprzgcs_..._source.changes>`
10. Update changelog minor version number and distribution name in the release directory to build for other distributions, and restart from step 8. Do not commit those changes.



Building with pbuilder-dist
===========================
pbuilder-dist is a wrapper that makes it easy to use pbuilder with many
different versions of Ubuntu and/or Debian.

Setup pbuilder-dist
-------------------
- Create the pbuilder environment for your desired distributions and architectures, e.g.:

        pbuilder-dist focal create

- To update a pbuilder environment:

        pbuilder-dist focal update --override-config --release-only

Build the debian package
------------------------
See the respective package directory on how the get the source.

- First build a source package if not already available:

		cd package
        debuild -S

- Build the binary package from the source package:

        pbuilder-dist focal build ../<package>.dsc

You will find the finished packages in ~/pbuilder/


