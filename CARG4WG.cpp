#include "CARG4WG.h"




CARG4WG::    CARG4WG(int nth)
{
  nthread = nth;
  fprintf(stderr, "fm->nthread: %d\n", nthread);
  nRec = 0;
  //mu = true;
  priqueue = (prioqueue_onepair*) new prioqueue_onepair;
}


CARG4WG::    ~CARG4WG()
{
  for (int i = 0; i != (int) nodes.size(); ++i) delete nodes[i];
  if (priqueue != NULL) free(priqueue);
 
}

bool operator < (const onepair &x, const onepair &y)
{
  return x.match < y.match;
}

void CARG4WG :: FindPath(int id) const
{
}


void CARG4WG :: CheckCount(void) const
{
  int nsite = data->GetnSite();
  for (int i = 0; i != nsite; ++i)
    {
      std::string s = "";
      for (int j = 0; j != (int) activenodes.size(); ++j) {
	int id  = activenodes[j];
	if (nodes[id]->ExistSite(i)) s = s + nodes[id]->GetSite(i);
      }

      int c0 =0, c1 = 0, c2 = 0;
      for (int k = 0; k != (int) s.size(); ++k)
	{
	  if (s[k] == '0') c0 ++;
	  if (s[k] == '1') c1 ++;
	  if (!NOTMISSING(s[k])) { c2 ++; fprintf(stderr, "%d: %d\n", k, s[k]);}
	}
      if (c0 != count0[i] || c1 != count1[i])
	{
	  fprintf(stderr, "error: %d: %d\t%d\t%s [%d,%d, %d]\n", i, count0[i], count1[i], s.c_str(), c0,c1, c2);
	  exit(1);
	}
    }
}


void CARG4WG :: ReadData(const CData *idata)
{

  data = idata;

  nodes.clear();
  activenodes.clear();

  count0.clear();
  count1.clear();

  int nsite = data->GetnSite();

  count0.resize(nsite, 0);
  count1.resize(nsite, 0);

  const vecstring *haps = data->GetHaps();

  for (int i = 0; i != (int) haps->size(); ++i)
    {
      const char *p = haps->at(i).c_str();
      for (int k = 0; k != nsite; ++k)
	{
	  if (p[k] == '0') count0[k] ++;
	  if (p[k] == '1') count1[k] ++;
	}
    }

  char* tempc = (char*)calloc(M20, sizeof(char));
  fprintf(stderr, "Read Data ......... \n");
  for (int i = 0; i != (int) haps->size(); ++i)
    {
      strcpy(tempc, haps->at(i).c_str());
      COneNode *e = new COneNode(NULL, NULL, 0, nsite-1, tempc, COPY);
      e->SetLeaf();
      Add(e);

    }
  free(tempc);
  for (int k = 0; k != nsite; ++k)
    if (POSSIBLEMUT(count0[k], count1[k])) mutlist.push_back(k);
}

void CARG4WG :: Add(COneNode* &onenode)
{

  std::vector<int> iret; //thread id
  iret.resize(nthread, 0);

  std::vector<pthread_t> threads; //pthreads
  threads.resize(nthread, 0);

  std::vector<onethread> ths; //data for one thread
  
  onethread e;
  e.nodes = &nodes;
  e.onenode = onenode;
  e.activenodes.clear();
  ths.resize(nthread, e);


  //add nodes into
  int nc = 0;
  for (int i = 0; i != (int) activenodes.size(); ++i)
    {
      int id = activenodes[i];
      if (onenode->overlap( nodes[id]->GetLeft(), nodes[id]->GetRight()))
    	  ths[nc ++ % nthread].activenodes.push_back(id);
    }
  for (int i = 0; i != (int) ths.size(); ++i)
    {
      int nr = (int) ths[i].activenodes.size();
      ths[i].match.resize(nr, 0);
      ths[i].mismatchpoint.resize(nr, 0);
      ths[i].isLeft.resize(nr, 0);
    }


  for (int i = 0; i != nthread; ++i)
    iret[i] = pthread_create(&threads[i], NULL, LeftMatch, (void*) &ths[i]);

  for (int i = 0; i != nthread; ++i) pthread_join(threads[i], NULL);


  for (int i = 0; i != (int) ths.size(); ++i)
    for (int j = 0; j != (int) ths[i].activenodes.size(); ++j)
      {
	int id = (int) ths[i].activenodes[j];
	onepair e;
	e.node1 = onenode;
	e.node2 = nodes[id];
	e.match = ths[i].match[j];
	e.mismatchpoint = ths[i].mismatchpoint[j];
	e.matchLeft = ths[i].isLeft[j];

	if (e.mismatchpoint == -1 && e.match > 0) 
		coallist.push_back(e);
	if (e.match > 0) priqueue->push(e);
      }

  onenode->SetID((int) nodes.size());
  nodes.push_back(onenode);
  activenodes.push_back(onenode->GetID());
}

void CARG4WG :: Mutation(int siteid, int id)
{
  bool mut = false;
  for (int i = 0; i != (int) activenodes.size(); ++i)
    {
      int nid = activenodes[i];
      COneNode *e = NULL;
      if (nodes[nid]->Mutation(siteid, id, e))
	{
	  RemoveActiveID( nodes[nid]->GetID()); //must be done before add e as nodes[nid] has no data
	  Add(e);
	  mut = true;
	  break;
	}
    }

  if (!mut && 1==1)
    {
      fprintf(stderr, "cannot find mutation\n");
      exit(1);
    }
  if (id == (int)'0') { count0[siteid]  --; count1[siteid] ++;}
  else {count1[siteid]  --; count0[siteid] ++;}
}


bool CARG4WG :: PossibleMutation(void)
{

  bool mut = false;
  std::random_shuffle(mutlist.begin(), mutlist.end());
  for (int ii = 0; ii != (int) mutlist.size(); ++ii)
    {
      int i = mutlist[ii];
      if (POSSIBLEMUT( count0[i], count1[i]))
	{
	  if (count1[i] == 1)	    Mutation(i, '1');
	  else Mutation(i, '0');
	  mut = true;
	}
    }
  mutlist.clear();
  return mut;
}




void CARG4WG :: RemoveActiveID(int id)
{
  for (int i = 0; i != (int) activenodes.size(); ++i)
    if (id == activenodes[i])
      {
	activenodes[i] = activenodes[ activenodes.size()-1 ];
	activenodes.resize(activenodes.size()-1);
	return  ;
      }
}


void CARG4WG :: Coal(COneNode *node1, COneNode* node2)
{

  int l1, r1, l2, r2;
  node1->GetLR(l1, r1);
  node2->GetLR(l2, r2);
  if (l1 < l2 || (l1 == l2 && r1 > r2))
    {
      COneNode *t = node1;
      node1 = node2;
      node2 = t;
    }

  node1->GetLR(l1, r1);
  node2->GetLR(l2, r2);

  char *p1 = node1->GetDataOut(); //get data out of node 1
  char *p  = node2->GetDataOut(); //get data out of node 2

  if (r2 < r1)
    p = (char*) realloc (p, (r1 - l2 + 3) * sizeof(char)); //extend p1

  p[ MAX(r1, r2) - l2 + 1] = '\0'; //

  int id1 = 0, id = l1 - l2;
  for (int i = l1; i <= r1; ++i)
    {
      if ((id <= (r2-l2)) && NOTMISSING(p[id]) && NOTMISSING(p1[id1]))
	{
	  if (p[id] == '0') count0[i] --;
	  else count1[i] --;

	  if (POSSIBLEMUT( count0[i], count1[i]))
	  { 	
		mutlist.push_back(i);
	  }
	}

      //if (!NOTMISSING(p[id]))
      p[id] = p1[id1];
      id1 ++;
      id  ++;
    }

  free(p1);

  COneNode *e = new COneNode(node1, node2, l2, MAX(r1, r2), p, NOCOPY);

  RemoveActiveID( node1->GetID() );
  RemoveActiveID( node2->GetID() );


  node1->SetParents(e, NULL);
  node2->SetParents(e, NULL);

  node1->Deactivate();
  node1->ResetData();

  node2->Deactivate();
  node2->ResetData();

//  fprintf(stderr,"Coal!!!!!!!!! \n");
  Add(e);
  
}



void CARG4WG :: Recom(COneNode *node1,  COneNode* node2, int matchpoint, bool matchLeft)
{

  if (node1->GetLength() > node2->GetLength()) //break the smaller node
    {
      COneNode *t = node1; node1 = node2; node2 = t;
    }


  COneNode *e1 = NULL, *e2 = NULL;

  node1->Break(matchpoint, e1, e2, matchLeft);
  RemoveActiveID(node1->GetID());
  
  Add(e1);
  e2->SetID(nodes.size()); nodes.push_back(e2);
  Coal(e2, node2);
}




bool CARG4WG :: PossibleCoal(void)
{

  while (!coallist.empty())
    {
      std::random_shuffle(coallist.begin(), coallist.end());
      onepair e = coallist.back();
      coallist.pop_back();
      COneNode *node1 = e.node1, *node2 = e.node2;
      if (node1->GetMark() || node2->GetMark()) continue;
      Coal(node1, node2);
      return true;
    }
  return false;

}



bool CARG4WG :: Recom(void)
{

if (priqueue->size() > 50000000  && priqueue->size() > activenodes.size() * activenodes.size() * 10)
    {
      prioqueue_onepair *temp = (prioqueue_onepair*) new prioqueue_onepair;
      while (!priqueue->empty())
	{
	  onepair e = priqueue->top();
	  priqueue->pop();
	  if (!e.node1->GetMark() && !e.node2->GetMark())  temp->push(e);
	}

      delete priqueue;
      priqueue = temp;
    }




  int len = 0;
  std::vector<onepair> qe;
    onepair temp;
    temp.match = 0;

  while (!priqueue->empty())
    {
      onepair e = priqueue->top();
      priqueue->pop();

      COneNode *node1 = e.node1, *node2 = e.node2;
      if (e.match == 0) continue;
      if (node1->GetMark() || node2->GetMark()) continue; // find a used node

      if (len == 0) len = e.match;
        if (e.match < len) { temp = e; break; }
      if (e.match == len) qe.push_back(e);
    }

  if (qe.size() == 0) return false;

  int i = rand() % qe.size();
  Recom(qe[i].node1, qe[i].node2, qe[i].mismatchpoint, qe[i].matchLeft);
  nRec++;
  for (int i = 0; i != (int) qe.size(); ++i)
    if (!qe[i].node1->GetMark() && !qe[i].node2->GetMark())  priqueue->push(qe[i]);
  if ((temp.match!= 0) && !temp.node1->GetMark() && !temp.node2->GetMark())  priqueue->push(temp);
  return true;
}



void CARG4WG :: BuildARG(void)
{
  int oldsize = 0;
  while (true)
    {
      if ((int) nodes.size()   > oldsize  + 10000)
	{
	  fprintf(stderr, "size = %d, active size = %d, nsite = %d, mul = %d, priqueue = %d, coal = %d\n", (int) nodes.size(), (int) activenodes.size(), (int)  data->GetnSite(), (int) mutlist.size(), (int) priqueue->size(), (int)  coallist.size());
	  oldsize = (int) nodes.size();
      }

      if (PossibleCoal()) continue;
      if (PossibleMutation()) continue;
      if (!Recom()) break;
    }
  fprintf(stderr, "\n**************************finish buiding ARG*************\n");
  fprintf(stderr, "Rec num %d\n", nRec);
    View();
}


void CARG4WG :: View(void) const
{
  for (int i = 0; i != (int) activenodes.size(); ++i)
    nodes[ activenodes[i] ]->View();
}

void CARG4WG :: Check(const COneNode *root, int siteid) const
{
  std::vector<const COneNode*> q;
  q.push_back(root);

  std::vector<int> getids;
  int i = 0;
  while (i < (int) q.size())
    {
      const COneNode *e = q[i++], *p = NULL;
      const COneNode *c1 = NULL, *c2 = NULL;

      while (1==1)
	{
	  e->GetChild(c1, c2);

	  int nc = 0;
	  if (c1 != NULL && c1->Contain(siteid))  { nc ++;  p = c1;}
	  if (c2 != NULL && c2->Contain(siteid))  { nc ++;  p = c2;}
	  if (nc == 1) e = p;
	  else break;
	}

      if (e->checkleaf()) {
	getids.push_back(e->GetID());
	continue;
      }
      if (c1 != NULL && c1->Contain(siteid))  q.push_back(c1);
      if (c2 != NULL && c2->Contain(siteid))  q.push_back(c2);
    }

  sort(getids.begin(), getids.end());

  bool error = false;
  int nhap = data->GetnHap();
  for (int i = 1; i < (int) getids.size(); ++i)
    if (getids[i] == getids[i-1]) error = true;

  if ((int)getids.size() != nhap || error)
    {
      fprintf(stderr, "getid size: %d\t nhap: %d \n", (int)getids.size(),nhap);
      fprintf(stderr, "error at site: %d\n", siteid);
      for (int i = 0; i != (int) getids.size(); ++i)
	fprintf(stderr, "%d\t", getids[i]);
      fprintf(stderr, "\n");
      exit(1);
    }
}

void CARG4WG :: Check(void) const
{
  COneNode *root = NULL;
  for (int i = 0; i != (int) nodes.size(); ++i)
    if (nodes[i]->checkroot())
      {
	root = nodes[i];
	break;
      }

  fprintf(stderr, "checking \n");
  for (int i = 0; i != data->GetnSite(); ++i) {
    if (i % 1000 == 0) printf("i = %d\n", i);
    Check(root, i);
  }
  fprintf(stderr, "ok in check\n");
}

void CARG4WG :: Write2ARG(FILE *fout)
{
  int nrm = 0;
  std::vector<const COneNode *> pp;
  pp.resize(4);
  for (int i = 0; i != (int) nodes.size(); ++i)
    if (nodes[i]->MutationNode())
      {
    
    fprintf(stderr, "\n>>>>>>> Mutation node %d \n", nodes[i]->GetID());
	nodes[i]->GetParents(pp[0], pp[1]);
      	nodes[i]->GetChild(pp[2], pp[3]);
      for (int j = 0; j != 4; ++j)
	if (pp[j] == NULL) fprintf(stderr, "\t-1");
	else fprintf(stderr, "\t%d", pp[j]->GetID());
	fprintf(stderr, "\n");

	nodes[i]->RemoveNode();
	delete nodes[i];
	nodes[i] = NULL;
	nrm ++;
      }

  fprintf(stderr, "rm node = %d --> %d\n", (int) nodes.size(), nrm);

  vecint poss;
  data->GetPos(poss);

  int l = 0;
  for (int i = 0; i != (int) nodes.size(); ++i)
    if (nodes[i] != NULL) nodes[l++] = nodes[i];
  nodes.resize(l);

  for (int i = 0; i != (int) nodes.size(); ++i)    nodes[i]->SetID(i);

  const vecstring *names = data->GetSampleNames();

  fprintf(fout, "sample %d\n", (int) names->size());
  fprintf(fout, "%s", names->at(0).c_str());
  for (int i = 1; i != (int) names->size(); ++i)
    fprintf(fout, "\t%s", names->at(i).c_str());




  fprintf(fout, "\n");
  std::vector<const COneNode *> p;
  p.resize(4);
  fprintf(fout, "nodes %d\n", (int) nodes.size());
  
  for (int i = 0;  i != (int) nodes.size(); ++i)
    {

      fprintf(fout, "%d\t%d\t%d", nodes[i]->GetID(), poss[nodes[i]->GetLeft()], poss[nodes[i]->GetRight()]);
      nodes[i]->GetParents(p[0], p[1]);
      nodes[i]->GetChild(p[2], p[3]);

      for (int j = 0; j != 4; ++j)
	if (p[j] == NULL) fprintf(fout, "\t-1");
	else fprintf(fout, "\t%d", p[j]->GetID());
      fprintf(fout, "\n");
    }

}


