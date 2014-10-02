
ALLDIRS=client server

.PHONY: all clean


all:
	cd server; qmake; $(MAKE)
	cd client; qmake; $(MAKE)


clean:
	cd server; $(MAKE) $@
	cd client; $(MAKE) $@
#	-@for dir in $(ALLDIRS); do $(MAKE) -C $$dir $@; done
