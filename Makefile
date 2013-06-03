CXX = clang++
CC = clang
CXXFLAGS = -c -std=c++11 -stdlib=libc++ -Wall
LINKFLAGS = -stdlib=libc++

SOURCES = json_value.cc json_object.cc json_parser.cc json_writer.cc

SHARED_LIB = $(CXX) -shared -dynamiclib $(LINKFLAGS)
STATIC_LIB = libtool -static

PathTransform = $(foreach obj,$($(1):.cc=.o),$(2)/$(obj))

debug/%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -o $@

release/%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -o $@

all: debugdir releasedir debug release

debugdir:
	mkdir -p debug
releasedir:
	mkdir -p release

debug: CXXFLAGS += -g
debug: $(call PathTransform,SOURCES,debug) debug/json_debug.o
	$(STATIC_LIB) $(call PathTransform,SOURCES,debug) debug/json_debug.o -o debug/libjsond.a

release: CXXFLAGS += -O4 -flto
release: $(call PathTransform,SOURCES,release)
#$(SHARED_LIB) $(call PathTransform,SOURCES,release) -o release/libjson.dylib
	$(STATIC_LIB) $(call PathTransform,SOURCES,release) -o release/libjson.a


test: debugdir debug test/tester.o
	$(CXX) $(LINKFLAGS) -Ldebug/ -ljsond test/tester.o -o test/tester

test/tester.o: test/tester.cc
	$(CXX) $(CXXFLAGS) -g test/tester.cc -o test/tester.o


.PHONY: clean
clean:
	rm -rf debug release test/tester.o test/tester bench/build


json_value.o: json_value.cc json_value.hpp json_string.hpp json_object.hpp
json_parser.o: json_parser.cc json_parser.hpp json_value.hpp json_string.hpp json_object.hpp
json_object.o: json_object.cc json_value.hpp json_string.hpp
json_writer.o: json_writer.cc json_writer.hpp
