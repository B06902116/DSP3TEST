#include <stdio.h>
#include "File.h"
#include "Prob.h"
#include "Ngram.h"
#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <iterator>
using namespace std;

#define LOG_ZERO -1024
#define MAXPERLINE 16384
typedef unsigned int unt;
Vocab voc;
Ngram *lm;
const VocabIndex NoHistory[] = {Vocab_None};
VocabIndex OneHistory[2];
typedef map<string, vector<string> > MAP;
typedef struct{
	int index;
	LogP p;
	string word;
}Node;

void init_disambig(const char* mapping_file, const char* lm_file, int order,MAP& mymap){
	
	
	//init lm
	lm = new Ngram(voc, order);
	File lmFile(lm_file, "r");
	lm->read(lmFile);
	lmFile.close();
	//init map
	FILE *fp = fopen(mapping_file,"r");
	char line[MAXPERLINE];
	while(fgets(line, sizeof(line),fp)!=NULL){
		line[strlen(line)-1]='\0';
		char *token = strtok(line, " ");
		string key = token;
		token = strtok(NULL, " ");
		while(token!=NULL){
			mymap[key].push_back(token);
			token = strtok(NULL, " ");
		}
	}
	mymap["<s>"].push_back("<s>");
	mymap["</s>"].push_back("</s>");
}

VocabIndex myGetIndex(const char *word){
	VocabIndex index = voc.getIndex(word);
	return (index == Vocab_None)? voc.getIndex(Vocab_Unknown) : index;
}
LogP getP(LogP p){
	return (p == LogP_Zero)? LOG_ZERO : p;
}
void disamLine(const vector<string>& w, FILE* fp, unt w_num, MAP& mymap){
	vector<vector<Node> > table;
	vector<Node> CandSet;
	
	OneHistory[1] = Vocab_None;
	w_num-=1;
	int i = -1;
	for(auto it = w.begin(); it != w.end(); it++){
		i -=-1;
		CandSet.clear();
		if(i == 0){
			for(auto j = mymap[*it].begin(); j!=mymap[*it].end();j++){
			VocabIndex cand = myGetIndex(j->data());
			LogP uni_p = getP(lm->wordProb(cand,NoHistory));
			CandSet.push_back((Node){0,uni_p,"<s>"});
			table.push_back(CandSet);
			}
		}
		else{
		for(auto j = mymap[*it].begin(); j!=mymap[*it].end();j++){
			LogP maxP = LogP_Zero;
			int maxcand = 0;
			//j is cand
			VocabIndex cand = myGetIndex(j->data());
			LogP uni_p = getP(lm->wordProb(cand, NoHistory));
			for(int k = 0;k<table[i-1].size();k++){
				OneHistory[0]=myGetIndex(table[i-1][k].word.data());
	
				LogP bi_p = getP(lm->wordProb(cand, OneHistory));
				if(uni_p == LOG_ZERO)
					bi_p = LOG_ZERO;
				LogP pcand = bi_p + table[i-1][k].p;
				if(pcand > maxP){
					maxP = pcand;
					maxcand = k;
				}
			}
			CandSet.push_back((Node){maxcand,maxP,*j});
		}
		table.push_back(CandSet);
		}
	}
	vector<string> outstring;
	int idx = 0;
	for(int i=table.size()-1;i>=0;i--){
		outstring.push_back(table[i][idx].word);
		idx = table[i][idx].index;
	}
	for(int i = outstring.size()-1;i>=0;i--){
		fprintf(fp,"%s%s",outstring[i].data(),(i == 0)? "\n" : " ");
	}
}
vector<string> sep_string(char line[]){
	vector<string> out_s;
	out_s.push_back("<s>");
	char *token = strtok(line, " ");
	while(token != NULL){	
		out_s.push_back(token);
		token = strtok(NULL, " ");
	}
	out_s.push_back("</s>");
	return out_s;
}
void solve(const char* input_text, const char* outFile, MAP& mymap){
	
	FILE* outputFile = fopen(outFile, "wb+");
	char line[512];
	FILE* inputFile = fopen(input_text,"r");
	while(fgets(line, sizeof(line),inputFile)!=NULL){
		line[strlen(line)-1]='\0';
		vector<string> in_string = sep_string(line);	
		disamLine(in_string, outputFile, in_string.size(), mymap);
	}
	fclose(outputFile);
	fclose(inputFile);
}

int main(int argc, char* argv[]){

	const char* input_text = argv[1];
	const char* mapping_file = argv[2];
	const char* lm_file = argv[3];
	const char* outFile = argv[4];
	MAP mymap;
	init_disambig(mapping_file, lm_file, 2, mymap);
	solve(input_text, outFile, mymap);
}
