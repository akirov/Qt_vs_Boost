
CXX=g++
CFLAGS+=-Wall

BOOST_LIB_SUFFIX=-mt
LINK_BOOST=-lboost_system$(BOOST_LIB_SUFFIX) -lboost_thread$(BOOST_LIB_SUFFIX) -lboost_signals$(BOOST_LIB_SUFFIX) -lpthread

EXEDIR=../exe
TARGET=$(EXEDIR)/server

SOURCES=$(wildcard *.cpp)
HEADERS=$(wildcard *.hpp)
OBJS=$(SOURCES:%.cpp=%.o)
DEPEND=.depend


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LINK_BOOST)


%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<


$(DEPEND): $(SOURCES) $(HEADERS)
	$(CXX) -MM $^ > $@

-include $(DEPEND)


clean:
	rm -f $(OBJS) $(DEPEND) $(TARGET)
