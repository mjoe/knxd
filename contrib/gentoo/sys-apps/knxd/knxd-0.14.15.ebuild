# Copyright 1999-2015 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $
# Author: Michael Kefeder (m.kefeder@gmail.com)
# Author: Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at)
# Author: Marc Joliet (marcec@gmx.de)

EAPI="5"

inherit eutils autotools git-r3 user

DESCRIPTION="Provides an interface to the EIB / KNX bus"
HOMEPAGE="https://github.com/Makki1/knxd"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS=""
IUSE="eibd ft12 tpuart eibnetip eibnetiptunnel eibnetipserver usb java dummy systemd"

DEPEND="dev-libs/libfmt
    dev-libs/libev
    usb? ( dev-libs/libusb )
    java? ( virtual/jdk )
    "

EGIT_REPO_URI="https://github.com/knxd/knxd.git"
EGIT_COMMIT="v${PV}"

src_prepare() {
    eautoreconf || die "eautotooling failed"
}

src_configure() {
    econf \
        $(use_enable systemd) \
        $(use_enable ft12) \
        $(use_enable tpuart) \
        $(use_enable eibnetip) \
        $(use_enable eibnetiptunnel) \
        $(use_enable eibnetipserver) \
        $(use_enable usb) \
        $(use_enable java) || die "econf failed"

}

src_compile() {
    emake || die "build of knxd failed"
}

src_install() {
    emake DESTDIR="${D}" install

    einfo "Installing init-script and config"

    sed -e "s|@SLOT@|${SLOT}|g" \
           "${FILESDIR}/${PN}.init" | newinitd - ${PN}-${SLOT}

    sed -e "s|@SLOT@|${SLOT}|g" \
           "${FILESDIR}/${PN}.confd" | newconfd - ${PN}-${SLOT}
}

pkg_setup() {
	enewgroup knxd
	enewuser knxd -1 -1 -1 "knxd"
}
