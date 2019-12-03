SRIPATH ?= /root/srilm-1.5.10
MACHINE_TYPE ?= i686-m64
INC_PATH ?= inc
SRC_PATH ?= src

CXX = g++
CXXFLAGS = -O2 -I$(SRIPATH)/include -I$(INC_PATH)
vpath lib%.a $(SRIPATH)/lib/$(MACHINE_TYPE)
vpath %.c $(SRC_PATH)
vpath %.cpp $(SRC_PATH)

TARGET = mydisambig tri_disambig
SRC = mydisambig.cpp
SRC2 = tri_disambig.cpp
OBJ = $(SRC:.cpp=.o)
OBJ2 = $(SRC2:.cpp=.o)
FROM ?= Big5-ZhuYin.map
TO ?= ZhuYin-Big5.map
DATASEG ?= test_data_seg
TESTDATA ?= test_data
RESULT1 ?= result#original result
RESULT2 ?= my_result
RESULT3 ?= tri_result
LMODEL ?= bigram.lm
TRILMODEL ?= trigram.lm
CNTFILE ?= lm.cnt
TRICNTFILE ?= trilm.cnt
TRAINDATA ?= corpus.txt
TRAINSEG ?= corpus_seg.txt
PRUNNUM ?= 5
.PHONY: all clean map

all: $(TARGET)

mydisambig: $(OBJ) -loolm -ldstruct -lmisc
	$(CXX) $(LDFLAGS) -o $@ $^

tri_disambig: $(OBJ2) -loolm -ldstruct -lmisc
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

check:
	@for i in $(shell seq 1 10); do \
		diff $(RESULT2)/$${i}.txt $(RESULT1)/$${i}.txt; \
	done;

run:
	mkdir -p $(RESULT2);
	@for i in $(shell seq 1 10); do \
		echo "Parsing $${i}.txt..."; \
		./mydisambig $(DATASEG)/$${i}.txt $(TO) $(LMODEL) $(RESULT2)/$${i}.txt; \
	done;
runtri:
	mkdir -p $(RESULT3);
	@for i in $(shell seq 1 10); do \
		echo "Parsing $${i}.txt..."; \
		./tri_disambig $(DATASEG)/$${i}.txt $(TO) $(TRILMODEL) $(RESULT3)/$${i}.txt $(PRUNNUM); \
	done;

map:
	@echo "Mapping starting..."
	python3 mapping.py $(FROM) $(TO)

makelm:
	perl separator_big5.pl $(TRAINDATA) > $(TRAINSEG)
	ngram-count -text $(TRAINSEG) -write $(CNTFILE) -order 2
	ngram-count -read $(CNTFILE) -lm $(LMODEL) -order 2 -unk

maketrilm:
	perl separator_big5.pl $(TRAINDATA) > $(TRAINSEG)
	ngram-count -text $(TRAINSEG) -write $(TRICNTFILE) -order 3
	ngram-count -read $(TRICNTFILE) -lm $(TRILMODEL) -order 3 -unk
separate:
	mkdir -p $(DATASEG)
	for i in $(shell seq 1 10); do \
		perl separator_big5.pl $(TESTDATA)/$${i}.txt > $(DATASEG)/$${i}.txt; \
	done

test:
	mkdir -p $(RESULT1);
	for i in $(shell seq 1 10); do \
		disambig -text $(DATASEG)/$${i}.txt -map $(TO) -lm $(LMODEL) -order 2 > $(RESULT1)/$${i}.txt; \
	done 

clean:
	$(RM) $(OBJ) $(TARGET)
