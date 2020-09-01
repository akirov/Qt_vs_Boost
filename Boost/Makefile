
ALLDIRS=client server

.PHONY: all clean


ifeq ($(OS),Windows_NT)
  ifneq ($(shell echo $$OSTYPE),cygwin)
    # Windows, but not Cygwin (MinGW)
    BOOST_LOCATION="D:/Qt/Tools/boost_1_73_0/mingw730_32"
    BOOST_LIB_SUFFIX=-mgw7-mt-x32-1_73
    PLATFORM_SPECIFIC_LINKS=-lws2_32 -lwsock32
    EXEDIR=..\\exe\\
    RM = del /F /Q
  else
    # Cygwin
    BOOST_LOCATION=/usr
    BOOST_LIB_SUFFIX=
    PLATFORM_SPECIFIC_LINKS=
    EXEDIR=../exe/
    RM = rm -rf
  endif
else
  # Linux
  BOOST_LOCATION=/usr
  BOOST_LIB_SUFFIX=
  PLATFORM_SPECIFIC_LINKS=
  EXEDIR=../exe/
  RM = rm -rf
endif


export


all:
	cd server && $(MAKE)
	cd client && $(MAKE)


clean:
	cd server && $(MAKE) $@
	cd client && $(MAKE) $@
#	-@for dir in $(ALLDIRS); do $(MAKE) -C $$dir $@; done
