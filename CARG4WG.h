#ifndef  __CARG4WG__
#define  __CARG4WG__

#include <stdio.h>
#include <stdlib.h>



#include <string>
#include <string.h>
#include <vector>
#include <time.h>
#include <ctime>

#include <queue>
#include <pthread.h>





#include "COneNode.h"
#include "Match.h"
#include "define.h"
#include "CData.h"
#include "utilities.h"

struct onepair
{
  COneNode* node1, *node2;
  int match;
  int mismatchpoint; //mismatch point
  bool matchLeft;
};

typedef std::priority_queue <onepair> prioqueue_onepair;
typedef std::vector <onepair> vec_onepair;

bool operator < (const onepair &x, const onepair &y);

class CARG4WG
{
 private:
  

  const CData *data;

  vecint activenodes;
  vecint mutlist; //list of possible mutation
  prioqueue_onepair *priqueue;
  vec_onepair coallist;
  vecint count0, count1;
  //vecpCOneNode nodetothread;
  //bool mu;

  int nthread;
  int nRec;
  //  int nsite, nsp;
  void Add(COneNode* &node);
  //void AddNodetoThread();

  void RemoveActiveID(int id);

  void Mutation(int siteid, int id);
  void Coal(COneNode *node1, COneNode* node2);

  bool PossibleMutation(void);
  bool PossibleCoal(void);
  bool Recom(void);

  void Recom(COneNode *node1, COneNode *node2, int siteid, bool matchLeft);
  void  CheckCount(void) const;

  void FindPath(int id) const;

  void Check(const COneNode *root, int siteid) const;

 public:

  vecpCOneNode nodes;

  CARG4WG(int nth);
  ~CARG4WG();

  void ReadData(const CData *idata);
  void BuildARG(void);
  void View(void) const;

  void ReadrmSNPs      (char *fn);
  void ReadselectedSNPs(char *fn);
  void Write2ARG(FILE *fout);

  void Check(void) const;
};


#endif
