CTEMPLATE_PREFIX ?= /usr/local
CTEMPLATE_LIBDIR ?= $(CTEMPLATE_PREFIX)/lib

oj_server: oj_server.cc $(wildcard *.hpp ../comm/*.h ../comm/*.hpp)
	g++ -std=c++11 -pthread $(CPPFLAGS) $(CXXFLAGS) $(shell pkg-config --cflags jsoncpp) $(shell mysql_config --include) -I$(CTEMPLATE_PREFIX)/include $< -o $@ $(LDFLAGS) -L$(CTEMPLATE_LIBDIR) -Wl,-rpath,$(CTEMPLATE_LIBDIR) -lctemplate -ljsoncpp $(shell mysql_config --libs) $(LDLIBS)

.PHONY: clean
clean:
	rm -f oj_server
