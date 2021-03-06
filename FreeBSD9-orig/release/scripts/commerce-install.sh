#!/bin/sh
#
# $FreeBSD: release/9.0.0/release/scripts/commerce-install.sh 75328 2001-04-08 23:09:21Z obrien $
#

if [ "`id -u`" != "0" ]; then
	echo "Sorry, this must be done as root."
	exit 1
fi
echo "Extracting commerce tarball into ${DESTDIR}/usr/local"
tar --unlink -xpzf commerce.tgz -C ${DESTDIR}/usr/local
exit 0
