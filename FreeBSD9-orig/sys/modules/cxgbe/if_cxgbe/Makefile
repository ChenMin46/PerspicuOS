#
# $FreeBSD: release/9.0.0/sys/modules/cxgbe/if_cxgbe/Makefile 223483 2011-06-23 20:31:52Z uqs $
#

CXGBE = ${.CURDIR}/../../../dev/cxgbe
.PATH: ${CXGBE} ${CXGBE}/common

KMOD = if_cxgbe
SRCS = t4_main.c t4_sge.c t4_l2t.c
SRCS+= t4_hw.c
SRCS+= device_if.h bus_if.h pci_if.h
SRCS+= opt_inet.h
SRCS+= opt_ofed.h

CFLAGS+= -I${CXGBE}

# Provide the timestamp of a packet in its header mbuf.
#CFLAGS+= -DT4_PKT_TIMESTAMP

.include <bsd.kmod.mk>
