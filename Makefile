CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -Wall -Isrc `icu-config-64 --cppflags`
LDFLAGS=-g
LDLIBS=-lre2 -lpthread `icu-config-64 --ldflags`
GTEST_CPPFLAGS=$(CPPFLAGS) -Isrc `gtest-config --cppflags`
GTEST_LDFLAGS=`gtest-config --ldflags`
GTEST_LDLIBS=$(LDLIBS) `gtest-config --libs`

SRCS=src/tokenizer.cc src/stemmer.cc src/porter2_stemmer.cpp src/bio.cc
OBJS=$(subst .cpp,.o,$(subst .cc,.o,$(SRCS)))

GTEST_SRCS=test/stemmer_test.cc test/run.cc
GTEST_OBJS=$(subst .cc,.o,$(GTEST_SRCS))

MAIN_SRCS=src/main.cc
MAIN_OBJS=$(subst .cc,.o,$(MAIN_SRCS))

all: src/token_regex.i twittok

check: $(OBJS) $(GTEST_OBJS)
	$(CXX) $(GTEST_LDFLAGS) -o test/run $(OBJS) $(GTEST_OBJS) $(GTEST_LDLIBS)
	test/run

twittok: $(OBJS) $(MAIN_OBJS)
	$(CXX) $(LDFLAGS) -o twittok $(OBJS) $(MAIN_OBJS) $(LDLIBS) 

src/token_regex.i: build-regex/generate-c++.rb
	build-regex/generate-c++.rb

depend: .depend

.depend: $(SRCS) $(GTEST_SRCS) $(MAIN_SRC)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^ >> ./.depend;

clean:
	$(RM) $(OBJS) $(GTEST_OBJS) twittok test/run

dist-clean: clean
	$(RM) *~ .depend

include .depend
