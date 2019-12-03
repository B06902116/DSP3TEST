#include <stdlib.h>
#include <iostream>
#include <queue>
#include "File.h"
#include "Prob.h"
#include "Ngram.h"
#include "LM.h"
#include "Vocab.h"
#include "VocabMap.h"
using namespace std;
#define CAND_NUM 1024
#define LOG_ZERO -128
#define MAXPERLINE 256
#define MAXPRON 20
typedef unsigned int unt;
typedef pair<int, LogP> pip;
Vocab voc, Zhuyin, Big5; 
Ngram *lm;
VocabMap *map;

int pron;

const VocabIndex NoHistory[] = {Vocab_None};
VocabIndex OneHistory[2];
VocabIndex TwoHistory[3];
void init_disambig(const char* mapping_file, const char* lm_file, int order){
	
	//init map
	map = new VocabMap(Zhuyin, Big5);
	File mapFile(mapping_file, "r");
	map->read(mapFile);
	mapFile.close();
	
	//init lm
	lm = new Ngram(voc, order);
	File lmFile(lm_file, "r");
	lm->read(lmFile);
	lmFile.close();
}

VocabIndex myGetIndex(const VocabIndex& big5_idx){
	VocabIndex index = voc.getIndex(Big5.getWord(big5_idx));
	return (index == Vocab_None)? voc.getIndex(Vocab_Unknown) : index;
}
LogP getP(LogP p){
	return (p == LogP_Zero)? LOG_ZERO : p;
}
struct compare
{	
	bool operator()(const pip& a, const pip& b)
	{
		return a.second < b.second;
	}
};
void disamLine(VocabString* w, FILE* fp, unt w_num){
	LogP prob[MAXPERLINE][CAND_NUM] = {{0.0}};
	VocabIndex IndexSet[MAXPERLINE][CAND_NUM];
	int BackTrack[MAXPERLINE][CAND_NUM];
	int SetNum[MAXPERLINE][CAND_NUM] = {{0}};
	int*** TrackSet = new int**[MAXPERLINE];
	for(int i = 0;i<MAXPERLINE;i++){
		TrackSet[i] = new int*[CAND_NUM];
		for(int j=0;j<CAND_NUM;j++){
			TrackSet[i][j] = new int[MAXPRON];
		}	
	}
	OneHistory[1] = Vocab_None;
	TwoHistory[2] = Vocab_None;
	Prob p;
	VocabIndex index;
	int CandNum[MAXPERLINE] = {};
	for(int i = 0; i < w_num; i++){
		VocabMapIter iter(*map, Zhuyin.getIndex(w[i]));
		iter.init();
		if(i == 0){//unigram
			while(iter.next(index, p)){
				VocabIndex cand = myGetIndex(index);
				LogP uni_p = getP(lm->wordProb(cand, NoHistory));
				prob[0][CandNum[0]] = uni_p;
				IndexSet[0][CandNum[0]] = index;
				BackTrack[0][CandNum[0]] = -1;
				int num = SetNum[0][CandNum[0]];
				TrackSet[0][CandNum[0]][num] = -1;
				CandNum[0]++;
			}
		}
		else if(i == 1){//bigram
			priority_queue<pip, vector<pip>, compare > pq;
			while(iter.next(index,p)){
				VocabIndex cand = myGetIndex(index);
				LogP uni_p = lm->wordProb(cand, NoHistory);
				LogP maxP = LogP_Zero;
				int maxCand = 0;
				for(int j = 0;j < CandNum[i-1];j++){
					OneHistory[0] = myGetIndex(IndexSet[i-1][j]);
					LogP bi_p = lm->wordProb(cand, OneHistory);
					if(uni_p == LogP_Zero && bi_p == LogP_Zero)
						bi_p = LOG_ZERO;
					LogP pcand = bi_p + prob[i-1][j];
					if(pcand > maxP){
					maxP = pcand;
					maxCand = j;
					}
					pq.push(make_pair(j,pcand));
					if(pq.size() > pron)
						pq.pop();
				}
				
				for(int j = 0;j < pq.size();j++){
				pip temp = pq.top();
				pq.pop();
				TrackSet[i][CandNum[i]][j] = temp.first;
				SetNum[i][CandNum[i]]++;
				}
				
				prob[i][CandNum[i]] = maxP;
				IndexSet[i][CandNum[i]] = index;
				BackTrack[i][CandNum[i]] = maxCand;
				CandNum[i]++;	
			}
		}
		else{//trigram
			priority_queue<pip, vector<pip>, compare > pq;
			while(iter.next(index,p)){
				VocabIndex cand = myGetIndex(index);
				LogP uni_p = lm->wordProb(cand, NoHistory);
				LogP maxP = LogP_Zero;
				int maxCand = 0;
				int q = 0;
				for(int j = 0;j < CandNum[i-1];j++){//bigram
					for(int k = 0;k < SetNum[i-1][j];k++){
					//for start
					int last = TrackSet[i-1][j][k];
					int t = TrackSet[1][0][0];
					TwoHistory[1] = myGetIndex(IndexSet[i-2][last]);
					TwoHistory[0] = myGetIndex(IndexSet[i-1][j]);
					OneHistory[0] = TwoHistory[1];
					LogP tri_p = lm->wordProb(cand, TwoHistory);
					LogP bi_p = lm->wordProb(TwoHistory[0], OneHistory);
					if(uni_p == LogP_Zero && bi_p == LogP_Zero && tri_p == LogP_Zero)
						tri_p = LOG_ZERO;
					LogP pcand = tri_p + bi_p + prob[i-2][last];
					if(pcand > maxP){
						maxP = pcand;
						maxCand = j;
					}
					
					pq.push(make_pair(j,pcand));
					if(pq.size() > pron)
						pq.pop();
					
					}//for end								
				}
				for(int j = 0; j < pq.size();j++){
				pip temp = pq.top();
				pq.pop();
				TrackSet[i][CandNum[i]][j] = temp.first;
				SetNum[i][CandNum[i]]++;
				}
				prob[i][CandNum[i]] = maxP;
				IndexSet[i][CandNum[i]] = index;
				BackTrack[i][CandNum[i]] = maxCand;
				CandNum[i]++;	
			}
		}
	}
	
	//backtracking
	LogP maxP = LogP_Zero;
	int maxCand;
	for(int i = 0;i < CandNum[w_num-1];i++){
		if(prob[w_num-1][i] > maxP){
			maxP = prob[w_num-1][i];
			maxCand = i;
		}	
	}
	VocabString OutPutString[1024];
	for(int i = w_num-1; i > 0; i--){
		OutPutString[i] = Big5.getWord(IndexSet[i][maxCand]);
		maxCand = BackTrack[i][maxCand];
	}
	OutPutString[0] = "<s>";
	OutPutString[w_num-1] = "</s>";
	for(int i = 0; i < w_num; i++)
		fprintf(fp, "%s%s", OutPutString[i], (i == w_num-1)? "\n" : " ");
	
}

void solve(const char* input_text, const char* outFile){
	File inputFile(input_text, "r");
	FILE* outputFile = fopen(outFile, "wb+");
	char* line;
	while(line = inputFile.getline()){
	VocabString w[maxWordsPerLine];
	unt w_num = Vocab::parseWords(line, &w[1], maxWordsPerLine) + 2;
	w[0] = "<s>";
	w[w_num-1] = "</s>";
	disamLine(w, outputFile, w_num);
	
	}
	inputFile.close();
	fclose(outputFile);
}

int main(int argc, char* argv[]){

	const char* input_text = argv[1];
	const char* mapping_file = argv[2];
	const char* lm_file = argv[3];
	const char* outFile = argv[4];
	pron = atoi(argv[5]);
	init_disambig(mapping_file, lm_file, 3);
	solve(input_text, outFile);
}