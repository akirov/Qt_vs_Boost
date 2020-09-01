
CXX=g++
CFLAGS+=-Wall

LINK_BOOST=-L$(BOOST_LOCATION)/lib -lboost_system$(BOOST_LIB_SUFFIX) -lboost_thread$(BOOST_LIB_SUFFIX) -lpthread $(PLATFORM_SPECIFIC_LINKS)

TARGET=$(EXEDIR)server

SOURCES=$(wildcard *.cpp)
HEADERS=$(wildcard *.hpp)
OBJS=$(SOURCES:%.cpp=%.o)
DEPEND=.depend


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LINK_BOOST)


%.o: %.cpp
	$(CXX) $(CFLAGS) -I$(BOOST_LOCATION)/include -c $<


$(DEPEND): $(SOURCES) $(HEADERS)
	$(CXX) -MM $^ > $@

-include $(DEPEND)


clean:
	$(RM) $(OBJS) $(DEPEND) $(TARGET) $(TARGET).exe
