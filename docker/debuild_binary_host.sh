#!/bin/sh

version=$1
dist=$2

tarfile=qrsspig_$version.orig.tar.xz
dir=qrsspig-$version

export GPG_TTY=/dev/console
gpg --quiet --import priv
gpg --quiet --import pub

#mkdir -p deb/$version/$dist
cd deb/$version/$dist
#cp ../$tarfile .
#tar xf $tarfile
cd $dir
#cp debian/control.$dist debian/control
debuild -b
