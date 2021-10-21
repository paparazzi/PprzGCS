
Before making a new release
===========================

+ Update `debian/changelog`
+ Edit `debian/rules` to set `VERSION` to the correct value.


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


