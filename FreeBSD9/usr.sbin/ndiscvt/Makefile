# $FreeBSD: release/9.0.0/usr.sbin/ndiscvt/Makefile 145485 2005-04-24 20:21:22Z wpaul $

.PATH:	${.CURDIR}/../../sys/compat/ndis

PROG=	ndiscvt
SRCS=	ndiscvt.c
SRCS+=	subr_pe.c
SRCS+=	inf.c inf-token.l inf-parse.y y.tab.h

MAN=	ndiscvt.8
MAN+=	ndisgen.8

WARNS?=	4

DPADD=  ${LIBL}
LDADD=  -ll

YFLAGS+=-v

CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/../../sys

CLEANFILES= y.output

FILES= windrv_stub.c
FILESNAME= windrv_stub.c
FILESDIR= /usr/share/misc

SCRIPTS= ndisgen.sh

.include <bsd.prog.mk>
