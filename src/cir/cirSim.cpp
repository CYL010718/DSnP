/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <climits>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/

bool vectorSort(vector<int>* a, vector<int>* b){
     return((*a)[0] < (*b)[0]);
}

void
CirMgr::randomSim()
{
  if(simGrps.empty()) initSimGrps();
  int simNum = 0;
  int failCount = 0;
  while(failCount < 2000){
   
    initSimValue();
    for(size_t j = 0; j < PIList.size(); ++j){
      getGate(PIList[j]) -> addSimValue(rnGen(INT_MAX));
      getGate(PIList[j]) -> setSimValue( (getGate(PIList[j]) -> getSimValue() << 32) + rnGen(INT_MAX));
    } 
    for(size_t j = 0; j < DFSList.size(); ++j){
      getGate(DFSList[j]) -> simulate();
    }
    int s = collectSimGrp(false);
    if(s == simGrps.size()) {
      ++failCount;
    }
   // cout << s << " " << simGrps.size() << endl;
    ++simNum;
  }
  sortSimGrps();
  assignSimGrps(false);
  outputSim(64);
  cout << simNum*64 << " patterns simulated." << endl;

}

void
CirMgr::fileSim(ifstream& patternFile)
{
  char str[InputNum+2];
  initSimValue();
  int count = 0;
  int simulatedNum = 0;
  while(patternFile >> str){
    //patternFile.getline(str,InputNum + 2, ' ');
    //cout << "strlen: " << strlen(str) <<  endl;
    if(strlen(str) == 0) break;
    if(count == 0) initSimValue();
    if(strlen(str) != InputNum){
      cerr << "Error: Pattern(" << str << ") length(" << strlen(str) << ") does not match the number of inputs("
      << InputNum << ") in a circuit!!" << endl;
      cout << simulatedNum << " patterns simulated." << endl;
      simGrps.clear();
      return;
    }
    for(size_t i = 0; i < InputNum; ++i){
      if(str[i] == '1')
        getGate(PIList[i]) -> addSimValue(pow(2,count));
      else if(str[i] != '0'){
        cerr << "Error: Pattern(" << str << ") contains a non-0/1 character('" << str[i] << "')." << endl;
        cout << simulatedNum << " patterns simulated." << endl;
        simGrps.clear();
        return;
      }
    }
    ++count;
    if(count == 64) {
      if(simGrps.empty()) initSimGrps();
      count = 0;
      for(size_t j = 0; j < DFSList.size(); ++j){
        getGate(DFSList[j]) -> simulate();
      }
      /*for(size_t j = 0; j < POList.size(); ++j)
        getGate(POList[j]) -> simulate();*/
      simulatedNum += 64;
      collectSimGrp(false);
    }
  }
  if(count != 0){
    if(simGrps.empty()) initSimGrps();
    for(size_t j = 0; j < DFSList.size(); ++j){
        getGate(DFSList[j]) -> simulate();
    }
    simulatedNum += count;
    collectSimGrp(false);
  }
  sortSimGrps();
  assignSimGrps(false);
  outputSim(count);
  cout << simulatedNum << " patterns simulated." << endl;
}

void
CirMgr::initSimValue(){
  for(size_t i = 0; i < PIList.size(); ++i){
    getGate(PIList[i]) -> resetSimValue();
  }
}

void
CirMgr::initSimGrps(){
  /*for(size_t i = 0 ; i < simGrps.size(); ++i)
    delete simGrps[i];*/
  simGrps.clear();
  vector<int>* v = new vector<int>();
  v -> reserve(DFSList.size() + 1);
  
  for(size_t i = 0; i < DFSList.size(); ++i){
    if(getGate(DFSList[i]) -> getTypeStr() == "AIG"){
      v -> push_back(DFSList[i]*2);
    }
  }
  v -> push_back(0);
  simGrps.push_back(v);
}

void
CirMgr::resetSimGrps(){
  for(size_t i = 0; i < simGrps.size(); ++i){
    for(size_t j = 0; j < simGrps[i] ->  size(); ++j){
      if(getGate((*simGrps[i])[j]/2) != 0)
        getGate((*simGrps[i])[j]/2) -> setFECgroup(0);
    }
    delete simGrps[i];
  }
  simGrps.clear();
}

int
CirMgr::collectSimGrp(bool isFraig){
  CirGate* thisGate;
  bool check = false;
  bool checkTraversed = true;
  int failCount = 0;

  //unordered_map<size_t, vector<int>*> grpHash;
  grpHash.reserve(simGrps[0] -> size());

  unordered_map<size_t, vector<int>*>::iterator it;
  vector<int>* newGrp;
  for(int i = simGrps.size() - 1; i  >= 0; --i){
    check = false;
    checkTraversed = true;
    grpHash.clear();
    if(simGrps[i] -> size()  == 0){
      delete simGrps[i];
      swap(simGrps[i], simGrps.back());
      simGrps.pop_back();
      continue;
    }
    for(size_t j = 0; j < simGrps[i] -> size(); ++j){
      (*simGrps[i])[j] = (*simGrps[i])[j] & ~0x1;
      thisGate = getGate((*simGrps[i])[j]/2);
      if(simGrps[i] -> size() == 1){
        thisGate -> setFECgroup(0);
        delete simGrps[i];
        swap(simGrps[i], simGrps.back());
        simGrps.pop_back();
        // simGrps.erase(simGrps.begin() + i);
        check = true;
        continue;
      }
      if(isFraig && (*simGrps[i])[0] == 0){
        check = true;
        continue;
      }
      if(thisGate == 0) {
        simGrps[i] -> erase(simGrps[i] -> begin() + j);
        --j;
        continue;
      }
      if(fullList[thisGate -> getID()] % 4 <= 1) checkTraversed = false;

      it = grpHash.find(thisGate -> getSimValue());
      if(it == grpHash.end()) it = grpHash.find(~(thisGate -> getSimValue()));
      if(it != grpHash.end()){
        it -> second -> push_back((*simGrps[i])[j]);
      }
      else{
        newGrp = new vector<int>();
        newGrp -> reserve(2);
        newGrp -> push_back((*simGrps[i])[j]);
        grpHash.insert(make_pair(thisGate -> getSimValue(),newGrp));
      }
    }
    if(check || grpHash.size() == 1) {
      if(grpHash.size() == 1) {
        //if(checkTraversed) delete simGrps[i];
        delete grpHash.begin() -> second;
        ++failCount;
      }
      continue;
    }
    delete simGrps[i];
    swap(simGrps[i], simGrps.back());
    simGrps.pop_back();
    it = grpHash.begin();
    if(!checkTraversed){
       while(it != grpHash.end()){
        if(it -> second -> size() > 1)
          simGrps.push_back((it -> second));
        else{
          getGate(it -> second -> front()/2) ->setFECgroup(0);
          delete it  -> second;
        }
        ++it;
      }
    }
   
  }
  return failCount;
}

void
CirMgr::assignSimGrps(bool isFraig){
  for(size_t i = 0; i < simGrps.size(); ++i){
    if(isFraig && i == 0) continue;
    for(size_t j = 0; j < simGrps[i] -> size(); ++j){
      if(getGate((*simGrps[i])[j] / 2) == 0) continue;
     // cout << "value: " << getGate((*simGrps[i])[j] / 2) -> getID() << endl;
      if(getGate((*simGrps[i])[j] / 2) -> getSimValue() != getGate((*simGrps[i])[0] / 2) -> getSimValue()) {
        ++(*simGrps[i])[j];
        getGate((*simGrps[i])[j]/2) -> setFECgroup((size_t)simGrps[i] + 1);
      }
      else
        getGate((*simGrps[i])[j]/2) -> setFECgroup((size_t)simGrps[i]);
    }
  }
}

void
CirMgr::sortSimGrps(){
  for(size_t i = 0; i < simGrps.size(); ++i){
    sort((*simGrps[i]).begin(), (*simGrps[i]).end());
  }
  sort(simGrps.begin(), simGrps.end(), vectorSort);
}

void
CirMgr::outputSim(int rows){
  if(_simLog == 0) return;
  size_t count = 1;
  for(int i = 0; i < 64; ++i){
    if(i == rows) return;
    for(size_t j = 0; j < PIList.size(); ++j){
      *_simLog << (getGate(PIList[j]) -> getSimValue() / count) % 2;
    }
    *_simLog << " ";
    for(size_t j = 0; j < POList.size(); ++j){
      *_simLog << (getGate(POList[j]) -> getSimValue() / count) % 2;
    }
    *_simLog << endl;
    count *= 2;
  }
}
/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
