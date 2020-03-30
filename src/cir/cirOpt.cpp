/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  size_t floatIdx = 0;
  size_t unUsedIdx = 0;
  CirGate* thisGate;
  CirGate* nextGate;
  getGate(0) -> setGlobalRef();
  for(size_t i = 0; i < unUsedList.size(); ++i){
    DFSweep(getGate(unUsedList[i]));
  }
  for(size_t i = 0; i < fullList.size(); ++i){
    thisGate = getGate(i);
    if(thisGate == 0) continue;
    if(thisGate -> getTypeStr() != "PI" && !(fullList[i] & isDFS)){
        cout << "Sweeping: " << thisGate -> getTypeStr() << "(" << thisGate -> getID() << ") removed...\n";
        if(thisGate -> getTypeStr() != "UNDEF") --AIGNum;
        delete thisGate; 
        fullList[i] = 0;
    }
    else if(thisGate -> getTypeStr() != "UNDEF"){
      for(size_t j = 0; j < thisGate -> getFanInSize(); ++j){
         nextGate = thisGate -> getFanIn(j);
         if(nextGate -> getTypeStr() == "UNDEF") {
            FloatList[floatIdx] = i;
            ++floatIdx;
            break;
         }
      }
       if(thisGate -> getFanOutSize() == 0 && thisGate -> getTypeStr() != "PO" && i != 0) {
         unUsedList[unUsedIdx] = i;
         ++unUsedIdx;
       }
    }
  }

  FloatList.resize(floatIdx);
  unUsedList.resize(unUsedIdx);
}

void
CirMgr::DFSweep(CirGate* thisGate){
  if(thisGate -> getTypeStr() == "UNDEF") return;
  CirGate* nextGate;
  for(size_t i = 0; i < thisGate -> getFanInSize(); ++i){
    nextGate = thisGate -> getFanIn(i);
    if((fullList[nextGate -> getID()] & isDFS) || nextGate -> getTypeStr() == "PI"){
      nextGate -> deleteFanOut(thisGate);
      continue;
    }
    if(!nextGate -> isGlobalRef()) DFSweep(nextGate);
  }
  thisGate -> setToGlobalRef();
}
// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  CirGate* thisGate;
  CirGate* fanIn_0;
  CirGate* fanIn_1;
  bool isInv_0, isInv_1;
  for(size_t i = 0; i < DFSList.size(); ++i){
    thisGate = getGate(DFSList[i]);
    if(thisGate == 0 || thisGate -> getTypeStr() != "AIG") continue;

    fanIn_0 = thisGate -> getFanIn(0);
    isInv_0 = thisGate -> isInv(0);

    fanIn_1 = thisGate -> getFanIn(1);
    isInv_1 = thisGate -> isInv(1);

    if(fanIn_0 == fanIn_1){
      if(isInv_0 == isInv_1){
        fanIn_0 -> deleteFanOut(thisGate);
        fanIn_0 -> deleteFanOut(thisGate);
        for(size_t j = 0; j < thisGate -> getFanOutSize(); ++j){
          thisGate -> getFanOut(j) -> replaceFanIn(thisGate, (size_t)fanIn_0 + (isInv_0 != thisGate -> isOutInv(j)));
          fanIn_0 -> pushFanOut((size_t)thisGate -> getFanOut(j) + (isInv_0 != thisGate -> isOutInv(j)));
        }  
        cout << "Simplifying: " << fanIn_0 -> getID() << " merging ";
        if(isInv_0) cout << "!";
        cout << thisGate -> getID() << "..." << endl;
      }
      else{
        fanIn_0 -> deleteFanOut(thisGate);
        fanIn_0 -> deleteFanOut(thisGate);
        for(size_t j = 0; j < thisGate -> getFanOutSize(); ++j){
          thisGate -> getFanOut(j) -> replaceFanIn(thisGate, (size_t)_const0 + thisGate -> isOutInv(j));
          _const0 -> pushFanOut((size_t)thisGate -> getFanOut(j) + thisGate -> isOutInv(j));
        }
        cout << "Simplifying: 0 merging ";
        cout << thisGate -> getID() << "..." << endl;
        if(fanIn_0 -> getTypeStr() == "UNDEF" && fanIn_0 -> getFanOutSize() == 0) {
          fullList[fanIn_0 -> getID()] = 0;
          delete fanIn_0;
        }
        if(fanIn_1 -> getTypeStr() == "UNDEF" && fanIn_1 -> getFanOutSize() == 0) {
          fullList[fanIn_1 -> getID()] = 0;
          delete fanIn_1;
        }
      }
      fullList[thisGate -> getID()] = 0;
      --AIGNum;
      delete thisGate;
    }
    else if(fanIn_0 == _const0 && isInv_0 == 0){
        fanIn_1 -> deleteFanOut(thisGate);
        _const0 -> deleteFanOut(thisGate);
        for(size_t j = 0; j < thisGate -> getFanOutSize(); ++j){
          thisGate -> getFanOut(j) -> replaceFanIn(thisGate, (size_t)_const0 + thisGate -> isOutInv(j));
          _const0 -> pushFanOut((size_t)thisGate -> getFanOut(j) + thisGate -> isOutInv(j));
        }
        
        cout << "Simplifying: 0 merging ";
        cout << thisGate -> getID() << "..." << endl;
        fullList[thisGate -> getID()] = 0;
        --AIGNum;
        delete thisGate;
        if(fanIn_1 -> getTypeStr() == "UNDEF" && fanIn_1 -> getFanOutSize() == 0) {
          fullList[fanIn_1 -> getID()] = 0;
          delete fanIn_1;
        }
    }
    else if(fanIn_1 == _const0 && isInv_1 == 0){
        fanIn_0 -> deleteFanOut(thisGate);
        _const0 -> deleteFanOut(thisGate);
        for(size_t j = 0; j < thisGate -> getFanOutSize(); ++j){
          thisGate -> getFanOut(j) -> replaceFanIn(thisGate, (size_t)_const0 + thisGate -> isOutInv(j));
          _const0 -> pushFanOut((size_t)thisGate -> getFanOut(j) + thisGate -> isOutInv(j));
        }
        
        cout << "Simplifying: 0 merging ";
        cout << thisGate -> getID() << "..." << endl;
        fullList[thisGate -> getID()] = 0;
        --AIGNum;
        delete thisGate;
        if(fanIn_0 -> getTypeStr() == "UNDEF" && fanIn_0 -> getFanOutSize() == 0) {
          fullList[fanIn_0 -> getID()] = 0;
          delete fanIn_0;
        }
    }
    else if(fanIn_0 == _const0 && isInv_0 == 1){
        fanIn_1 -> deleteFanOut(thisGate);
        _const0 -> deleteFanOut(thisGate);
        for(size_t j = 0; j < thisGate -> getFanOutSize(); ++j){
          thisGate -> getFanOut(j) -> replaceFanIn(thisGate, (size_t)fanIn_1 + (isInv_1 != thisGate -> isOutInv(j)));
          fanIn_1 -> pushFanOut((size_t)thisGate -> getFanOut(j) + (isInv_1 != thisGate -> isOutInv(j)));
        }
       
        cout << "Simplifying: " << fanIn_1 -> getID() << " merging ";
        if(isInv_1) cout << "!";
        cout << thisGate -> getID() << "..." << endl;
        fullList[thisGate -> getID()] = 0;
        --AIGNum;
        delete thisGate;
    }
    else if(fanIn_1 == _const0 && isInv_1 == 1){
         fanIn_0 -> deleteFanOut(thisGate);
        _const0 -> deleteFanOut(thisGate);
        for(size_t j = 0; j < thisGate -> getFanOutSize(); ++j){
          thisGate -> getFanOut(j) -> replaceFanIn(thisGate, (size_t)fanIn_0 + (isInv_0 != thisGate -> isOutInv(j)));
          fanIn_0 -> pushFanOut((size_t)thisGate -> getFanOut(j) + (isInv_0 != thisGate -> isOutInv(j)));
        }
       
        cout << "Simplifying: " << fanIn_0 -> getID() << " merging ";
        if(isInv_0) cout << "!";
        cout << thisGate -> getID() << "..." << endl;
        fullList[thisGate -> getID()] = 0;
         --AIGNum;
        delete thisGate;
    }
  }

  DFSList.resize(0);
  unUsedList.resize(0);
  BuildDFSList();
  for(size_t i = 0; i < fullList.size(); ++i) {
     if(getGate(i)  == 0 || getGate(i)  -> getTypeStr() == "UNDEF") continue;
     if(getGate(i) -> getFanOutSize() == 0 && getGate(i)-> getTypeStr() != "PO" && i != 0) unUsedList.push_back(i);
  }
  
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
