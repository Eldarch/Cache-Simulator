#include<stdio.h>
#include <time.h>

void main(int argc, char* argv[]){

  //Enter Associativity (1, 2, 4, 8, or 16)
  int associativity = 2; //1 is direct mapped //also called set size
  //Enter Replacement Policy ('l' for LRU, 'r' for Random, or 'n' for NMRU+Random)
  char rplcPolcy = 'l';
  //Enter Cache Size in KB (16, 32, 64, 128, or 256)
  int cacheSize = 16;
  //Enter Block Size in words (1 - 8)
  int blockSize = 8; //8 words = 32 bytes
  int numBlocks = (cacheSize * 1024) / blockSize;
  int sets = numBlocks / associativity;
  unsigned long cache[sets][associativity]; //this will hold everything in the cache
  unsigned long blocks[sets][associativity]; //this will hold the blocks in the cache for comparison

  if(associativity == 1 && rplcPolcy == 'n') { //direct mapped cant be NMRU
    rplcPolcy = 'r';
  }
  //printf("%d\n", sets);
  
  int lruIndex[sets][associativity]; //need to keep track of LRU maybe
  int mruIndex = 0; //need to keep track of MRU maybe
  
  //initializing everything in the arrays to 0

  for (int i = 0; i < sets; i++)
  {
    for (int j = 0; j < associativity; j++)
    {
      cache[i][j] = 0;
      blocks[i][j] = 0;
    }
  }
  for (int i = 0; i < sets; i++)
  {
    for (int j = associativity - 1; j >= 0; j--)
    {
      lruIndex[i][j] = j; //[7,6,5,4,3,2,1,0],[7,6,5,4,3,2,1,0],[7,6,5,4,3,2,1,0] index 7 is the LRU
      //printf("%d\n", lruIndex[i][j]);
    }
  }
  
  srand(time(NULL)); //for the random and NMRU+Random replacement policies
  
  int b = log2(blockSize); // number of block offset bits: right most bits
  //printf("\n%d", b);
  int s = log2(cacheSize / blockSize); // number of index bits: middle bits
  //printf("%d\n", s);
  int t = 32 - s - b; // number of tag bits: left most bits
  //printf("%d\n", t);


  int numAccesses = 0;
  double extraNumAccesses = 0;
  int cacheHits = 0;
  int cacheMisses = 0;
  int accessState = 0; //will be a 1 if we hit, 0 if we miss

  unsigned long backwardsAccessAddr[32];
  unsigned long accessAddr[32]; // the address from the memory trace store in the bitset;
  //initializing to 0
  for (int op = 0; op < 32; op++)
    {
      backwardsAccessAddr[op] = 0;
      accessAddr[op] = 0;
    }
  
  unsigned long tagBits[t]; //will hold the tag bits
  unsigned long indexBits[s]; //will hold the index bits
  unsigned long offsetBits[b]; //will hold the offset bits
  unsigned long blockBits[t+s]; //will hold the tag and index bits


  //Going through the zip file and reading the lines
  FILE * f;
  char fname[50] = "tracefile.txt";
  printf("Enter file name 'tracefile.txt'\n");
  scanf("%s", fname);
  long diff;
  unsigned long ea = 0;
  unsigned long temp = 0;
  unsigned long tempDif = 0;

  f = fopen(fname,"r");

  
  unsigned int lines = 1500000;
  for(int i = 0; i < lines && (fscanf(f,"%ld",&diff) != EOF); i++) {
    numAccesses++; //should approach 1500000
    extraNumAccesses++;
    ea = ea + diff;
    //&temp = ea;
    //printf("%ld\n", ea);
    //printf("%p \n",ea);
    temp = ea;
    tempDif = diff;
    
    for(int j = 0; tempDif > 0; j++) { //getting the bit version of the address, but its backwards
      backwardsAccessAddr[j] = tempDif % 2;
      //printf("%d", backwardsAccessAddr[j]);
      tempDif /= 2;
    }
    int counter = 31; // for reversing
    for(int m = 0; m < 32; m++) { // reversing the bit address array
      //printf("%lu", backwardsAccessAddr[counter]);
      accessAddr[m] = backwardsAccessAddr[counter];
      //printf("%lu", accessAddr[m]);
      counter--;
    }
    
    int numTagBits = 0;
    int numIndexBits = 0;
    int numOffsetBits = 0;
    int numBlockBits = 0;

    //Splitting the 32 bits into the tag, index, offset, and block bits
    for(int kp = 0; kp < 32; kp++) {
      //printf("%d\n", kp);
      //printf("%lu", accessAddr[kp]);
      //printf("tag\n");
      if (numTagBits < t) {
        //printf("%lu", accessAddr[k]);
        tagBits[numTagBits] = accessAddr[kp];
        blockBits[numBlockBits] = accessAddr[kp];
        numTagBits++;
        numBlockBits++;
        continue;
      }
      //printf("index\n");
      if (numIndexBits < s) {
        indexBits[numIndexBits] = accessAddr[kp];
        blockBits[numBlockBits] = accessAddr[kp];
        numIndexBits++;
        numBlockBits++;
        //printf("%lu", accessAddr[k]);
        continue;
      }
      //printf("offset\n");
      if (numOffsetBits < b) {
        offsetBits[numOffsetBits] = accessAddr[kp];
        numOffsetBits++;
        //printf("%lu", accessAddr[k]);
      }
    }
    //printf("\n");
    // for(int z = 0; z < t + s; z++) {
    //   printf("%lu", blockBits[z]);
    // }
    //printf("\n");


    //Now I wanna turn my block bits into decimal form for comparisons in the next section
    unsigned long blockInt = 0;
    unsigned long aw = t + s - 1;
    for(int aq = 0; aq < t + s; aq++) {
      blockInt += blockBits[aw] * pow(2, aq);
      aw--;
    }
    //printf("%d\n", blockInt);


    
    //Gonna figure out what set ea address belongs to. Split max ea/temp into sets number of equal parts with %
    //We will also check that set to see if temp is missing or a hit
    unsigned long numOfPossBlocksPerSet = (pow(2,32) - 1) / sets;
    //printf("%d\n", blockInt);
    int setNum = blockInt % sets;
    
    //printf("%d\n", setNum);
    for(int yu = 0; yu < associativity; yu++) {
      if(blocks[setNum][yu] == blockInt) { //thats a hit
        accessState = 1;
        if(rplcPolcy == 'n') { //need to update MRU index
          mruIndex = yu;
        }
        if(rplcPolcy == 'l') { //need to update LRU index
          //need to push everything in the lru array down 1 starting at yu. The MRU index will be index 0 in lruIndex[][]. LRU index is in index associativity - 1
          int tempIndex = 0;
          for (int we = 0; we < associativity; we++) {
            if (lruIndex[setNum][we] == yu) { //this is the index of yu where we need to push the upper array down
              tempIndex = we;
            }
          }
          
          for(int gh = tempIndex; gh > 0; gh--) {
            int tempLRU = lruIndex[setNum][gh - 1];
            lruIndex[setNum][gh] = tempLRU;
          }
          lruIndex[setNum][0] = yu;
          //printf("\n%d", yu);
          // for(int jk = 0; jk < associativity; jk++) {
          //   printf("\n%d", lruIndex[setNum][jk]);
          // }
        }
        break;
      }
      //else thats a miss, do nothing. The access state is already a 0
    }

    if(accessState == 1) { //was a hit
      ++cacheHits; //add a cache hit
      //printf("%d\n", cacheHits);
      accessState = 0; //reset access state
      continue;
    }
    else { //was a miss
      ++cacheMisses; //add a cache miss
      //printf("%d\n", cacheMisses);
      if(rplcPolcy == 'l') { //'l' for LRU, 'r' for Random, or 'n' for NMRU+Random
        int victimIndex = lruIndex[setNum][associativity - 1];
        blocks[setNum][victimIndex] = blockInt;
        cache[setNum][victimIndex] = diff;
      }
      else if(rplcPolcy == 'r') {
        int victimIndex = rand() % associativity;
        blocks[setNum][victimIndex] = blockInt;
        cache[setNum][victimIndex] = diff;
      }
      else if(rplcPolcy == 'n') {
        int victimIndex = rand() % associativity;
          while(victimIndex == mruIndex) { //need to keep on choosing random index until its not the MRU
            victimIndex = rand() % associativity;
          }
        blocks[setNum][victimIndex] = blockInt;
        cache[setNum][victimIndex] = diff;
      }
    // }
    }
  }

  float hitRate = cacheHits / extraNumAccesses;
  float missRate = cacheMisses / extraNumAccesses;
  
  if(rplcPolcy == 'l') {
    printf("%s%d%s\n", "Cache_Size: ", cacheSize, " KBytes");
    printf("%s%d%s%d%s\n", "Block_Size: ", blockSize, " words (", blockSize * 4, " bytes)");
    printf("%s%d%s\n", "Associativity: ", associativity, " way");
    printf("%s\n", "Replacement_Policy: LRU");
    printf("%s%d\n", "Total_Number_of_Accesses: ", numAccesses);
    printf("%s%d\n", "Cache_Hits: ", cacheHits);
    printf("%s%d\n", "Cache_Misses: ", cacheMisses);
    printf("%s%.3f\n", "Cache_Hit_Rate: ", hitRate);
    printf("%s%.3f\n", "Cache_Miss_Rate: ", missRate);
  }
  else if(rplcPolcy == 'r') {
    printf("%s%d%s\n", "Cache_Size: ", cacheSize, " KBytes");
    printf("%s%d%s%d%s\n", "Block_Size: ", blockSize, " words (", blockSize * 4, " bytes)");
    printf("%s%d%s\n", "Associativity: ", associativity, " way");
    printf("%s\n", "Replacement_Policy: Random");
    printf("%s%d\n", "Total_Number_of_Accesses: ", numAccesses);
    printf("%s%d\n", "Cache_Hits: ", cacheHits);
    printf("%s%d\n", "Cache_Misses: ", cacheMisses);
    printf("%s%.3f\n", "Cache_Hit_Rate: ", hitRate);
    printf("%s%.3f\n", "Cache_Miss_Rate: ", missRate);
  }
  else if(rplcPolcy == 'n') {
    printf("%s%d%s\n", "Cache_Size: ", cacheSize, " KBytes");
    printf("%s%d%s%d%s\n", "Block_Size: ", blockSize, " words (", blockSize * 4, " bytes)");
    printf("%s%d%s\n", "Associativity: ", associativity, " way");
    printf("%s\n", "Replacement_Policy: NMRU+Random");
    printf("%s%d\n", "Total_Number_of_Accesses: ", numAccesses);
    printf("%s%d\n", "Cache_Hits: ", cacheHits);
    printf("%s%d\n", "Cache_Misses: ", cacheMisses);
    printf("%s%.3f\n", "Cache_Hit_Rate: ", hitRate);
    printf("%s%.3f\n", "Cache_Miss_Rate: ", missRate);
  }
  // for(int i = 0; i < sets; i++) {
  //   for (int j = 0; j < associativity; j++) {
  //     printf("\n%lu", cache[i][j]);
  //   }
  // }
  return;
}
