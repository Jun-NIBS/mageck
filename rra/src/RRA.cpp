/*
 *  RRA.c
 *  Implementation of Robust Rank Aggregation (RRA)
 *
 *  Created by Han Xu on 20/12/13.
 *  Modified by Wei Li.
 *  07/2014: updated the constants to accomodate pathway names
 *  Copyright 2013 Dana Farber Cancer Institute. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define NDEBUG
#include <assert.h>
#include "math_api.h"
#include "words.h"
#include "rvgs.h"
#include "rngs.h"

//C++ functions
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
using namespace std;

#define MAX_NAME_LEN 10000           //maximum length of item name, group name or list name
#define CDF_MAX_ERROR 1E-10        //maximum error in Cumulative Distribution Function estimation in beta statistics
#define MAX_GROUP_NUM 100000       //maximum number of groups
#define MAX_LIST_NUM 1000          //maximum number of list 
#define RAND_PASS_NUM 100          //number of passes in random simulation for computing FDR

#define MAX_WORD_NUM 1000        //maximum number of word

//WL: define boolean variables
//typedef int bool;
//#define true 1
//#define false 0

int PRINT_DEBUG=0;

typedef struct
{
	char name[MAX_NAME_LEN];       //name of the item
	int listIndex;                 //index of list storing the item
	double value;                  //value of measurement
	double percentile;             //percentile in the list
	double prob;			//The probability of each sgRNA; added by Wei
} ITEM_STRUCT;

typedef struct 
{
	char name[MAX_NAME_LEN];       //name of the group
	ITEM_STRUCT *items;            //items in the group
	int itemNum;                   //number of items in the group
  int maxItemNum;                // max number of items
	double loValue;                //lo-value in RRA
	double fdr;                    //false discovery rate
} GROUP_STRUCT;

typedef struct
{
	char name[MAX_NAME_LEN];       //name of the list
	double *values;                //values of items in the list, used for sorting
	int itemNum;                   //number of items in the list
  int maxItemNum;               //max item number
} LIST_STRUCT;

//Read input file. File Format: <item id> <group id> <list id> <value>. Return 1 if success, -1 if failure
int ReadFile(char *fileName, GROUP_STRUCT *groups, int maxGroupNum, int *groupNum, LIST_STRUCT *lists, int maxListNum, int *listNum);

//Save group information to output file. Format <group id> <number of items in the group> <lo-value> <false discovery rate>
int SaveGroupInfo(char *fileName, GROUP_STRUCT *groups, int groupNum);

//Process groups by computing percentiles for each item and lo-values for each group
int ProcessGroups(GROUP_STRUCT *groups, int groupNum, LIST_STRUCT *lists, int listNum, double maxPercentile);

//QuickSort groups by loValue
void QuickSortGroupByLoValue(GROUP_STRUCT *groups, int start, int end);

//Compute False Discovery Rate based on uniform distribution
int ComputeFDR(GROUP_STRUCT *groups, int groupNum, double maxPercentile, int numOfRandPass);

//print the usage of Command
void PrintCommandUsage(const char *command);


//Compute lo-value based on an array of percentiles
int ComputeLoValue(double *percentiles,     //array of percentiles
				   int num,                 //length of array
				   double *loValue,         //pointer to the output lo-value
				   double maxPercentile);   //maximum percentile, computation stops when maximum percentile is reached

//WL: modification of lo_value computation
int ComputeLoValue_Prob(double *percentiles,     //array of percentiles
				   int num,                 //length of array
				   double *loValue,         //pointer to the output lo-value
				   double maxPercentile,   //maximum percentile, computation stops when maximum percentile is reached
				  double *probValue); // probability of each prob, must be equal to the size of percentiles

//split strings
int stringSplit(string str, string delim, vector<string> & v){
  int start=0;
  int pos=str.find_first_of(delim,start);
  v.clear();
  while(pos!=str.npos){
    if(pos!=start)
      v.push_back(str.substr(start,pos-start));
     start=pos+1;
     pos=str.find_first_of(delim,start);
  }
  if(start<str.length())
    v.push_back(str.substr(start,str.length()-start));
  return v.size();
}

int main (int argc, const char * argv[]) 
{
	int i,flag;
	GROUP_STRUCT *groups;
	int groupNum;
	LIST_STRUCT *lists;
	int listNum;
	char inputFileName[1000], outputFileName[1000];
	double maxPercentile;
	
	//Parse the command line
	if (argc == 1)
	{
		PrintCommandUsage(argv[0]);
		return 0;
	}
	
	inputFileName[0] = 0;
	outputFileName[0] = 0;
	maxPercentile = 0.1;
	
	for (i=2;i<argc;i++)
	{
		if (strcmp(argv[i-1], "-i")==0)
		{
			strcpy(inputFileName, argv[i]);
		}
		if (strcmp(argv[i-1], "-o")==0)
		{
			strcpy(outputFileName, argv[i]);
		}
		if (strcmp(argv[i-1], "-p")==0)
		{
			maxPercentile = atof(argv[i]);
		}
	}
	
	if ((inputFileName[0]==0)||(outputFileName[0]==0))
	{
		printf("Command error!\n");
		PrintCommandUsage(argv[0]);
		return -1;
	}
	
	if ((maxPercentile>1.0)||(maxPercentile<0.0))
	{
		printf("maxPercentile should be within 0.0 and 1.0\n");
		printf("program exit!\n");
		return -1;
	}
	
	groups = (GROUP_STRUCT *)malloc(MAX_GROUP_NUM*sizeof(GROUP_STRUCT));
	lists = (LIST_STRUCT *)malloc(MAX_LIST_NUM*sizeof(LIST_STRUCT));
	assert(groups!=NULL);
	assert(lists!=NULL);
	
	printf("reading input file...");
	
	flag = ReadFile(inputFileName, groups, MAX_GROUP_NUM, &groupNum, lists, MAX_LIST_NUM, &listNum);
	
	if (flag<=0)
	{
		printf("\nfailed.\n");
		printf("program exit!\n");
		
		return -1;
	}
	else
	{
		printf("done.\n");
	}
	
	printf("computing lo-values for each group...");
	
	if (ProcessGroups(groups, groupNum, lists, listNum, maxPercentile)<=0)
	{
		printf("\nfailed.\n");
		printf("program exit!\n");
		
		return -1;
	}
	else
	{
		printf("done.\n");
	}
	
	printf("computing false discovery rate...");
	
	if (ComputeFDR(groups, groupNum, maxPercentile, RAND_PASS_NUM*groupNum)<=0)
	{
		printf("\nfailed.\n");
		printf("program exit!\n");
		
		return -1;
	}
	else
	{
		printf("done.\n");
	}
	
	printf("save to output file...");
	
	if (SaveGroupInfo(outputFileName, groups, groupNum)<=0)
	{
		printf("\nfailed.\n");
		printf("program exit!\n");
		
		return -1;
	}
	else
	{
		printf("done.\n");
	}
	
	printf("finished.\n");
	
	free(groups);
	
	for (i=0;i<listNum;i++)
	{
		free(lists[i].values);
	}
	free(lists);

	return 0;

}

//print the usage of Command
void PrintCommandUsage(const char *command)
{
	//print the options of the command
	printf("%s - Robust Rank Aggreation.\n", command);
	printf("usage:\n");
	printf("-i <input data file>. Format: <item id> <group id> <list id> <value> [<probability>]\n");
	printf("-o <output file>. Format: <group id> <number of items in the group> <lo-value> <false discovery rate>\n");
	printf("-p <maximum percentile>. RRA only consider the items with percentile smaller than this parameter. Default=0.1\n");
	printf("example:\n");
	printf("%s -i input.txt -o output.txt -p 0.1 \n", command);
	
}

//Read input file. File Format: <item id> <group id> <list id> <value>. Return 1 if success, -1 if failure

int ReadFile(char *fileName, GROUP_STRUCT *groups, int maxGroupNum, int *groupNum, LIST_STRUCT *lists, int maxListNum, int *listNum)
{
	//FILE *fh;
	int i,j,k;
	//char **words, *tmpS, *tmpS2;
	int wordNum;
	int totalItemNum;
	int tmpGroupNum, tmpListNum;
	//char tmpGroupName[MAX_NAME_LEN], tmpListName[MAX_NAME_LEN], tmpItemName[MAX_NAME_LEN];
	double tmpValue;
  
  //char **subwords;
	double sgrnaProbValue;
	
	//words = AllocWords(MAX_WORD_NUM, MAX_NAME_LEN+1);
	//subwords = AllocWords(MAX_WORD_NUM, MAX_NAME_LEN+1);
	
	//assert(words!=NULL);
	
	//if (words == NULL)
	//{
	//	return -1;
	//}
	
	//tmpS = (char *)malloc(MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char));
	//tmpS2 = (char *)malloc(MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char));
	
	//assert(tmpS!=NULL && tmpS2!=NULL);
	
	ifstream fh;
  fh.open(fileName);
  if(!fh.is_open()){
    cerr<<"Error opening "<<fileName<<endl;
  }
  string oneline;
  getline(fh,oneline);
  
  vector<string> vwords;
  vector<string> vsubwords;
  wordNum=stringSplit(oneline," \t\r\n\v",vwords);
	//Read the header row to get the sample number
	//fgets(tmpS, MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char), fh);
	//wordNum = StringToWords(words, tmpS,MAX_WORD_NUM, MAX_NAME_LEN+1,  " \t\r\n\v\f");
	
	assert(wordNum == 4 | wordNum == 5);
	
	if (wordNum != 4 && wordNum != 5)
	{
		cerr<<"Input file format: <item id> <group id> <list id> <value> [<prob>]\n";
    fh.close();
		return -1;
	}
	
	//read records of items
	
	tmpGroupNum = 0;
	tmpListNum = 0;
	totalItemNum = 0;
	
	//fgets(tmpS, MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char), fh);
	//wordNum = StringToWords(words, tmpS, MAX_NAME_LEN+1, MAX_WORD_NUM, " \t\r\n\v\f");
  getline(fh,oneline);
  wordNum=stringSplit(oneline," \t\r\f\v",vwords);
	
	//WL
 int subWordNum;
 subWordNum=0;

	while ((wordNum>=4 )&&(!fh.eof()))
	{
		//strcpy(tmpItemName, words[0]);
		//strcpy(tmpGroupName, words[1]);
		//strcpy(tmpListName, words[2]);
    
    // separate the group name by ","
		// modified by WL 
    //strcpy(tmpS,tmpGroupName);
   	//subWordNum=StringToWords(subwords,tmpS,MAX_NAME_LEN+1,MAX_WORD_NUM,",");
    subWordNum=stringSplit(vwords[1],",",vsubwords);
    assert(subWordNum>0);
    
		for(k=0;k<subWordNum;k++)
    {
	  		for (i=0;i<tmpGroupNum;i++)
	  		{
	  			if (!strcmp(vsubwords[k].c_str(), groups[i].name))
	  			{
	  				break;
	  			} 
	  		}
	  	
	  		if (i>=tmpGroupNum)
	  		{
	  			strcpy(groups[tmpGroupNum].name, vsubwords[k].c_str());
	  			groups[tmpGroupNum].itemNum = 1;
	  			tmpGroupNum ++;
	  			if (tmpGroupNum >= maxGroupNum)
	  			{
	  				printf("too many groups. maxGroupNum = %d\n", maxGroupNum);
	  				return -1;
	  			}
	  		}
	  		else
	  		{
	  			groups[i].itemNum++;
	  		}
		}

		for (i=0;i<tmpListNum;i++)
		{
			if (!strcmp(vwords[2].c_str(), lists[i].name))
			{
				break;
			}
		}
		
		if (i>=tmpListNum)
		{
			strcpy(lists[tmpListNum].name, vwords[2].c_str());
			lists[tmpListNum].itemNum = 1;
			tmpListNum ++;
			if (tmpListNum >= maxListNum)
			{
				printf("too many lists. maxListNum = %d\n", maxListNum);
				return -1;
			}
		}
		else
		{
			lists[i].itemNum++;
		}
		
		totalItemNum++;
		
    getline(fh,oneline);
    wordNum=stringSplit(oneline," \t\r\f\v",vwords);
		//fgets(tmpS, MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char), fh);
		//wordNum = StringToWords(words, tmpS, MAX_NAME_LEN+1, MAX_WORD_NUM, " \t\r\n\v\f");
	}
	
	fh.close();
	
	for (i=0;i<tmpGroupNum;i++)
	{
    //printf("Group %d items:%d\n",i,groups[i].itemNum);
		groups[i].items = (ITEM_STRUCT *)malloc(groups[i].itemNum*sizeof(ITEM_STRUCT));
		groups[i].maxItemNum = groups[i].itemNum;
		groups[i].itemNum = 0;
	}
	
	for (i=0;i<tmpListNum;i++)
	{
    //printf("List %d items:%d\n",i,lists[i].itemNum);
		lists[i].values = (double *)malloc(lists[i].itemNum*sizeof(double));
		lists[i].maxItemNum = lists[i].itemNum;
		lists[i].itemNum = 0;
	}
	
  fh.open(fileName);
  if(!fh.is_open()){
    cerr<<"Error opening "<<fileName<<endl;
  }
	
	//Read the header row to get the sample number
  getline(fh,oneline);
	//fgets(tmpS, MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char), fh);
	
	//read records of items
	
  getline(fh,oneline);
  wordNum=stringSplit(oneline," \t\r\n\v",vwords);
	//fgets(tmpS, MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char), fh);
	//wordNum = StringToWords(words, tmpS, MAX_NAME_LEN+1, MAX_WORD_NUM, " \t\r\n\v\f");
	
	while ((wordNum>=4)&&(!fh.eof()))
	{
		//strcpy(tmpItemName, words[0]);
		//strcpy(tmpGroupName, words[1]);
		//strcpy(tmpListName, words[2]);
		tmpValue = atof(vwords[3].c_str());
		//WL
		//parsing prob column, if available
		if (wordNum > 4)
		{
			sgrnaProbValue=atof(vwords[4].c_str());
    }
		else
		{
			sgrnaProbValue=1.0;
		}


		// WL: separate the group name by ","
		//strcpy(tmpS2,words[1]);
		//subWordNum=StringToWords(subwords,tmpS2,MAX_NAME_LEN+1,MAX_WORD_NUM,",");
    subWordNum=stringSplit(vwords[1],",",vsubwords);
		assert(subWordNum>0);
		
  		for (j=0;j<tmpListNum;j++)
  		{
  			if (!strcmp(vwords[2].c_str(), lists[j].name))
  			{
  				break;
  			}
  		}
    	assert(j<tmpListNum);

		lists[j].values[lists[j].itemNum] = tmpValue;
 		lists[j].itemNum ++;
    assert(lists[j].itemNum <=lists[j].maxItemNum);
		//for(k=0;k<subWordNum;k++)
		for(k=0;k<subWordNum;k++)
   {
      	//printf("gene: %s, word: %s\n",tmpItemName,subwords[k]);
  			for (i=0;i<tmpGroupNum;i++)
  			{
  				if (!strcmp(vsubwords[k].c_str(), groups[i].name))
  				{
  					break;
  				} 
  			}
  		
  		
  			assert(i<tmpGroupNum);
  		
  			//strcpy(groups[i].items[groups[i].itemNum].name,tmpItemName);
  			strcpy(groups[i].items[groups[i].itemNum].name,vwords[0].c_str());
  			groups[i].items[groups[i].itemNum].value = tmpValue;
  			groups[i].items[groups[i].itemNum].prob= sgrnaProbValue;
  			groups[i].items[groups[i].itemNum].listIndex = j;
  			groups[i].itemNum ++;
      	assert(groups[i].itemNum <=groups[i].maxItemNum);
  		
      	//printf("round %d, group %d itemnum: %d (max %d), list %d itemnum: %d (max %d)\n",k,i,groups[i].itemNum,groups[i].maxItemNum,j,lists[j].itemNum,lists[j].maxItemNum);
      
    }
		
		//fgets(tmpS, MAX_WORD_NUM*(MAX_NAME_LEN+1)*sizeof(char), fh);
		//wordNum = StringToWords(words, tmpS, MAX_NAME_LEN+1, MAX_WORD_NUM, " \t\r\n\v\f");
    getline(fh,oneline);
    wordNum=stringSplit(oneline," \t\r\f\v",vwords);
   	//printf("wordnum:%d,read one line length:%d\n",wordNum,strlen(tmpS));
	}
	
	fh.close();
	//fclose(fh);
	
	printf("%d items\n%d groups\n%d lists\n", totalItemNum, tmpGroupNum, tmpListNum);
	
	*groupNum = tmpGroupNum;
	*listNum = tmpListNum;
	
	//FreeWords(words, MAX_WORD_NUM);
	//free(tmpS);
	//free(tmpS2);
	//FreeWords(subwords, MAX_WORD_NUM);
	
	return totalItemNum;
	
}

//Save group information to output file. Format <group id> <number of items in the group> <lo-value> <false discovery rate>
int SaveGroupInfo(char *fileName, GROUP_STRUCT *groups, int groupNum)
{
	FILE *fh;
	int i;
	
	fh = (FILE *)fopen(fileName, "w");
	
	if (!fh)
	{
		printf("Cannot open %s.\n", fileName);
		return -1;
	}
	
	fprintf(fh, "group_id\titems_in_group\tlo_value\tFDR\n");
	
	for (i=0;i<groupNum;i++)
	{
		fprintf(fh, "%s\t%d\t%10.4e\t%f\n", groups[i].name, groups[i].itemNum, groups[i].loValue, groups[i].fdr);
	}
	
	fclose(fh);
	
	return 1;
}

//Process groups by computing percentiles for each item and lo-values for each group
//groups: genes
//lists: a set of different groups. Comparison will be performed on individual list
int ProcessGroups(GROUP_STRUCT *groups, int groupNum, LIST_STRUCT *lists, int listNum, double maxPercentile)
{
	int i,j;
	int listIndex, index1, index2;
	int maxItemPerGroup;
	double *tmpF;
	double *tmpProb;
	bool isallone; // check if all the probs are 1; if yes, do not use accumulation of prob. scores
	
	maxItemPerGroup = 0;

  PRINT_DEBUG=1;
	
	for (i=0;i<groupNum;i++)
	{
		if (groups[i].itemNum>maxItemPerGroup)
		{
			maxItemPerGroup = groups[i].itemNum;
		}
	}
	
	assert(maxItemPerGroup>0);
	
	tmpF = (double *)malloc(maxItemPerGroup*sizeof(double));
	tmpProb = (double *)malloc(maxItemPerGroup*sizeof(double));
	
	for (i=0;i<listNum;i++)
	{
		QuicksortF(lists[i].values, 0, lists[i].itemNum-1);
	}
	
	for (i=0;i<groupNum;i++)
	{
		//Compute percentile for each item
		
		isallone=true;
		for (j=0;j<groups[i].itemNum;j++)
		{
			listIndex = groups[i].items[j].listIndex;
			
			index1 = bTreeSearchingF(groups[i].items[j].value-0.000000001, lists[listIndex].values, 0, lists[listIndex].itemNum-1);
			index2 = bTreeSearchingF(groups[i].items[j].value+0.000000001, lists[listIndex].values, 0, lists[listIndex].itemNum-1);
			
			groups[i].items[j].percentile = ((double)index1+index2+1)/(lists[listIndex].itemNum*2);
			tmpF[j] = groups[i].items[j].percentile;
			tmpProb[j]=groups[i].items[j].prob;
			if(tmpProb[j]!=1.0)
			{
				isallone=false;
			}
		}
    if(groups[i].itemNum<=1)
    {
      isallone=true;
    }
    //printf("Gene: %s\n",groups[i].name);
		if(isallone)
		{
			ComputeLoValue(tmpF, groups[i].itemNum, &(groups[i].loValue), maxPercentile);
		}
		else
		{
			ComputeLoValue_Prob(tmpF, groups[i].itemNum, &(groups[i].loValue), maxPercentile, tmpProb);
		}
	}

	free(tmpF);
	free(tmpProb);
	
	return 1;
}

//Compute lo-value based on an array of percentiles. Return 1 if success, 0 if failure
int ComputeLoValue(double *percentiles,     //array of percentiles
				   int num,                 //length of array
				   double *loValue,         //pointer to the output lo-value
				   double maxPercentile)   //maximum percentile, computation stops when maximum percentile is reached
{
	int i;
	double *tmpArray;
	double tmpLoValue, tmpF;
	
	assert(num>0);
	
	tmpArray = (double *)malloc(num*sizeof(double));
	
	if (!tmpArray)
	{
		return -1;
	}
	
	memcpy(tmpArray, percentiles, num*sizeof(double));
	
	QuicksortF(tmpArray, 0, num-1);
	
	tmpLoValue = 1.0;
	
	for (i=0;i<num;i++)
	{
		if ((tmpArray[i]>maxPercentile)&&(i>0))
		{
			break;
		}
		tmpF = BetaNoncentralCdf((double)(i+1),(double)(num-i),0.0,tmpArray[i],CDF_MAX_ERROR);
		if (tmpF<tmpLoValue)
		{
			tmpLoValue = tmpF;
		}
	}
	
	*loValue = tmpLoValue;

	free(tmpArray);
	
	return 0;
	
}

//Compute lo-value based on an array of percentiles, by considering the probability of each sgRNAs. 
//Return 1 if success, 0 if failure
//Modified by Wei Li
int ComputeLoValue_Prob(double *percentiles,     //array of percentiles
				   int num,                 //length of array
				   double *loValue,         //pointer to the output lo-value
				   double maxPercentile,   //maximum percentile, computation stops when maximum percentile is reached
				  double *probValue) // probability of each prob, must be equal to the size of percentiles
{
	int i, pid;
	double *tmpArray;
	double tmpLoValue, tmpF;

	// used to calculate cumulative probability
	int real_i;
	int c_num;
	double c_prob;
	double accuLoValue;

	assert(num>0);
	
	tmpArray = (double *)malloc(num*sizeof(double));
	
	if (!tmpArray)
	{
		return -1;
	}
	
	memcpy(tmpArray, percentiles, num*sizeof(double));
	
	QuicksortF(tmpArray, 0, num-1);
  if(PRINT_DEBUG) printf("probs:");
  for(i=0;i<num;i++)
  {
    if(PRINT_DEBUG) printf("%f,",probValue[i]);
  }
  if(PRINT_DEBUG) printf("\n");
	
	
	accuLoValue=0.0;
	for (pid=1;pid<(1<<num);pid++)
	{
		// decoding the selection
		tmpLoValue = 1.0;
		c_num=0;
		real_i=0;
		c_prob=1.0;
		for(i=0;i<num;i++)
		{
			if ( (1<<i)&pid )
			{
				c_num = c_num +1;
				c_prob= c_prob*probValue[i];
			}
			else
			{
				c_prob=c_prob*(1.0-probValue[i]);
			}
		}
		for (i=0;i<num;i++)
		{
			if ( ((1<<i)&pid) ==0)
			{// if this sgRNA is selected?
				continue;
			}
			if ((tmpArray[i]>maxPercentile)&&(i>0))
			{
				break;
			}
			// Beta (a,b, 0, frac)
			tmpF = BetaNoncentralCdf((double)(real_i+1),(double)(c_num-i),0.0,tmpArray[i],CDF_MAX_ERROR);
			if (tmpF<tmpLoValue)
			{
				tmpLoValue = tmpF;
			}
			real_i = real_i + 1;
		}
    if(PRINT_DEBUG) printf("pid: %d, prob:%e, score: %f\n",pid,c_prob,tmpLoValue);
		accuLoValue=accuLoValue+tmpLoValue*c_prob;
	}
  if(PRINT_DEBUG) printf("total: %f\n",accuLoValue);
	
	*loValue = accuLoValue;

	free(tmpArray);
	
	return 0;
	
}


//Compute False Discovery Rate based on uniform distribution
int ComputeFDR(GROUP_STRUCT *groups, int groupNum, double maxPercentile, int numOfRandPass)
{
	int i,j,k;
	double *tmpPercentile;
	int maxItemNum = 0;
	int scanPass = numOfRandPass/groupNum+1;
	double *randLoValue;
	int randLoValueNum;

	//WL
	double *tmpProb;
	double isallone;
	
	for (i=0;i<groupNum;i++)
	{
		if (groups[i].itemNum>maxItemNum)
		{
			maxItemNum = groups[i].itemNum;
		}
	}
	
	assert(maxItemNum>0);
	
	tmpPercentile = (double *)malloc(maxItemNum*sizeof(double));
	tmpProb= (double *)malloc(maxItemNum*sizeof(double));
	
	randLoValueNum = groupNum*scanPass;
	
	assert(randLoValueNum>0);
	
	randLoValue = (double *)malloc(randLoValueNum*sizeof(double));
	
	randLoValueNum = 0;
	
	PlantSeeds(123456);
  
  PRINT_DEBUG=0;
	
	for (i=0;i<scanPass;i++)
	{
		for (j=0;j<groupNum;j++)
		{
			isallone=true;
			for (k=0;k<groups[j].itemNum;k++)
			{
				tmpPercentile[k] = Uniform(0.0, 1.0);
				tmpProb[k]=groups[j].items[k].prob;
				if(tmpProb[k]!=1.0)
				{
					isallone=false;
				}
			}
      if(groups[j].itemNum<=1)
      {
        isallone=true;
      }
			
			if(isallone)
			{
				ComputeLoValue(tmpPercentile, groups[j].itemNum,&(randLoValue[randLoValueNum]), maxPercentile);
			}
			else
			{
				ComputeLoValue_Prob(tmpPercentile, groups[j].itemNum,&(randLoValue[randLoValueNum]), maxPercentile,tmpProb);
			}
			
			randLoValueNum++;
		}
	}
	
	QuicksortF(randLoValue, 0, randLoValueNum-1);
						  
	QuickSortGroupByLoValue(groups, 0, groupNum-1);
	
	for (i=0;i<groupNum;i++)
	{
		//groups[i].fdr = (double)(bTreeSearchingF(groups[i].loValue-0.000000001, randLoValue, 0, randLoValueNum-1)
		//						 +bTreeSearchingF(groups[i].loValue+0.000000001, randLoValue, 0, randLoValueNum-1)+1)
		//						/2/randLoValueNum/((double)i+0.5)*groupNum;
		groups[i].fdr = (double)(bTreeSearchingF(groups[i].loValue-0.000000001, randLoValue, 0, randLoValueNum-1)
								 +bTreeSearchingF(groups[i].loValue+0.000000001, randLoValue, 0, randLoValueNum-1)+1)
								/2/randLoValueNum/((double)i+1.0)*groupNum;
	}
	
	if (groups[groupNum-1].fdr>1.0)
	{
		groups[groupNum-1].fdr = 1.0;
	}
	
	for (i=groupNum-2;i>=0;i--)
	{
		if (groups[i].fdr>groups[i+1].fdr)
		{
			groups[i].fdr = groups[i+1].fdr;
		}
	}
	
	free(tmpPercentile);
	free(tmpProb);
	free(randLoValue);
	
	return 1;
}

//QuickSort groups by loValue
void QuickSortGroupByLoValue(GROUP_STRUCT *groups, int lo, int hi)
{
	int i=lo, j=hi;
	GROUP_STRUCT tmpGroup;
	double x=groups[(lo+hi)/2].loValue;
	
	if (hi<lo)
	{
		return;
	}
	
    //  partition
    while (i<=j)
    {    
		while ((groups[i].loValue<x)&&(i<=j))
		{
			i++;
		}
		while ((groups[j].loValue>x)&&(i<=j))
		{
			j--;
		}
        if (i<=j)
        {
			memcpy(&tmpGroup,groups+i,sizeof(GROUP_STRUCT));
			memcpy(groups+i,groups+j,sizeof(GROUP_STRUCT));
			memcpy(groups+j,&tmpGroup,sizeof(GROUP_STRUCT));
            i++; j--;
        }
    } 
	
    //  recursion
    if (lo<j) QuickSortGroupByLoValue(groups, lo, j);
    if (i<hi) QuickSortGroupByLoValue(groups, i, hi);
	
}
