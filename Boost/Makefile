
ALLDIRS=client server

.PHONY: all clean


all:
	cd server; $(MAKE)
	cd client; $(MAKE)


clean:
	cd server; $(MAKE) $@
	cd client; $(MAKE) $@
#	-@for dir in $(ALLDIRS); do $(MAKE) -C $$dir $@; done
