##
##  Makefile -- Build procedure for sample senstive_filter Apache module
##  Autogenerated via ``apxs -n senstive_filter -g''.
##

builddir=.
top_srcdir=
top_builddir=/usr/share/apache2
include /usr/share/apache2/build/special.mk

#   the used tools
APXS=apxs
APACHECTL=apachectl

#   additional defines, includes and libraries
#DEFS=-Dmy_define=my_value
#INCLUDES=-Imy/include/dir
#LIBS=-Lmy/lib/dir -lmylib

#   the default target
all: local-shared-build

#   install the shared object file into Apache 
install: install-modules-yes

#   cleanup
clean:
	-rm -f mod_senstive_filter.o mod_senstive_filter.lo mod_senstive_filter.slo mod_senstive_filter.la 

#   simple test
test: reload
	lynx -mime_header http://localhost/senstive_filter

#   install and activate shared object by reloading Apache to
#   force a reload of the shared object file
reload: install restart

#   the general Apache start/restart/stop
#   procedures
start:
	$(APACHECTL) start
restart:
	$(APACHECTL) restart
stop:
	$(APACHECTL) stop
