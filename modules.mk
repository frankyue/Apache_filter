mod_senstive_filter.la: mod_senstive_filter.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_senstive_filter.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_senstive_filter.la
