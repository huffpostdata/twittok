CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lre2 -lpthread

SRCS=src/tokenizer.cc src/main.cc
OBJS=$(subst .cc,.o,$(SRCS))

all: src/token_regex.i twittok

twittok: $(OBJS)
	$(CXX) $(LDFLAGS) -o twittok $(OBJS) $(LDLIBS) 

src/token_regex.i: build-regex/generate-c++.rb
	build-regex/generate-c++.rb

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^ >> ./.depend;

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) *~ .depend

include .depend
