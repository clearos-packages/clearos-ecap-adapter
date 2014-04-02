AM_CPPFLAGS = -I$(top_srcdir)/src

lib_LTLIBRARIES = clearos-ecap-adapter.la

clearos_ecap_adapter_la_SOURCES = ecap-adapter.cpp
clearos_ecap_adapter_la_LDFLAGS = -module -avoid-version
clearos_ecap_adapter_la_LIBADD = -lecap

DISTCLEANFILES = autoconf.h

