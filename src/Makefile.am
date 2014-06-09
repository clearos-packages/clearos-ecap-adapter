AM_CPPFLAGS = -I$(top_srcdir)/src

noinst_HEADERS = expat-xml.h

lib_LTLIBRARIES = libclearos-ecap-adapter.la

libclearos_ecap_adapter_la_SOURCES = ecap-adapter.cpp expat-xml.cpp
libclearos_ecap_adapter_la_LDFLAGS = -module -avoid-version
libclearos_ecap_adapter_la_LIBADD = -lecap

DISTCLEANFILES = autoconf.h

