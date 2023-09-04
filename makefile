.SILENT:
# .IGNORE:

# g++ arguments
sem := -pthread
fil := -lstdc++fs
std := -std=c++17
dbg := -g -pg # gdb and gprof
err := -fmax-errors=1
g++args := $(sem) $(fil) $(std) $(dbg) $(err)

# Folders
src := src/
sqr := src/Sequencer
obj := obj/
out :=
VPATH := $(src) $(obj) $(sqr)

# Files
xfiles  := annihilate server client SequencerTest
sfiles  := Token Message DirPermit Subsequence Sequence Sequencer SqrParser
ofiles  := $(addsuffix .o,$(xfiles) Parseable $(sfiles)) Shr.o
code    := $(xfiles) $(ofiles)
scripts := script.sh stupid.sh

# Command templates
pfix    = echo $(1)$@ ; g++ $(g++args) -o
link    = $(call pfix, Linking.....) $(out)$@ $(addprefix $(obj),$(^F))
compile = $(call pfix, Compiling...) $(obj)$@ -c $(filter %.cpp,$^)

all: $(xfiles)

# Utilities
clean:
	rm $(obj)*.o $(xfiles) -f

test: .latestMakefileRule.txt
	echo Testing "$$(cat .latestMakefileRule.txt)"
	./"$$(cat .latestMakefileRule.txt)"

$(scripts):
	scripts/$@

# All xfiles are substrings in ofiles, but not vice-versa
$(code):
	if [[ "$(xfiles)" =~ "$@" ]]; then $(link); else $(compile); fi
	echo $@ | tr -d "\n" > .latestMakefileRule.txt

# Linking prerequisites
SequencerTest : SequencerTest.o       $(addsuffix .o,Parseable $(sfiles))
server        : server.o        Shr.o $(addsuffix .o,Parseable $(sfiles))
client        : client.o        Shr.o $(addsuffix .o,Parseable $(sfiles))
annihilate    : annihilate.o    Shr.o

# Compiling prerequisites
Parseable.o     : Parseable.cpp     Parseable.h
SequencerTest.o : SequencerTest.cpp
server.o        : server.cpp        Shr.h Sequencer.h
client.o        : client.cpp        Shr.h Sequencer.h
Shr.o           : Shr.cpp           Shr.h Sequencer.h
annihilate.o    : annihilate.cpp

.SECONDEXPANSION:
$(addsuffix .o,$(sfiles)) : Parseable.o $$(subst .o,.cpp,$$@) $$(subst .o,.h,$$@)