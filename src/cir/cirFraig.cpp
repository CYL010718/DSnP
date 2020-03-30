/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <unordered_map>
#include <chrono>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed


void
CirMgr::strash()
{
  CirGate* thisGate;
  CirGate* mergeGate;
  size_t fanIn_0;
  size_t fanIn_1;
  unordered_map<HashKey, CirGate*, HashFunction>::iterator it;
  for(size_t i = 0; i < DFSList.size(); ++i){
    thisGate = getGate(DFSList[i]);
    if(thisGate == 0 || thisGate -> getTypeStr() != "AIG") continue;
   
    fanIn_0 = (size_t)(thisGate -> getFanIn(0)) + thisGate -> isInv(0);
    fanIn_1 = (size_t)(thisGate -> getFanIn(1)) + thisGate -> isInv(1);
   

    //cout << "ji" << endl;
    it = hashMap.find(HashKey(fanIn_0, fanIn_1));

    if(it == hashMap.end()){
      hashMap.insert(make_pair(HashKey(fanIn_0, fanIn_1),thisGate));
    }
    else{
      mergeGate = it -> second;
      cout << "Strashing: " << mergeGate -> getID() << " merging " << thisGate -> getID() << "...\n";
      --AIGNum;
      for(size_t j = 0; j < thisGate ->  getFanOutSize(); ++j){
        thisGate -> getFanOut(j) -> replaceFanIn(thisGate, (size_t)mergeGate + (thisGate -> isOutInv(j)));
        mergeGate -> pushFanOut((size_t)thisGate -> getFanOut(j) + thisGate -> isOutInv(j));   
      }
      fullList[thisGate -> getID()] = 0;
      delete thisGate;
    }
  }
  hashMap.clear();
 // cout << "out of loop" << endl;
  DFSList.resize(0);
  FloatList.resize(0);
  unUsedList.resize(0);
  BuildDFSList();
  buildFloat();
}

void
CirMgr::fraig()
{
  CirGate* thisGate;
  CirGate* mergeGate;
  SatSolver solver;
  bool isFECInv;
  bool result;
  solver.initialize();
  Var v;
  initSimValue();
  int SATCount = 0;
  fullList[0] += 2;
  v = solver.newVar();
  _const0 -> setVar(v);
  for(size_t i = 0; i < DFSList.size(); ++i){
    v = solver.newVar();
    thisGate = getGate(DFSList[i]);
    //cout << "hi" << endl;
    if(thisGate == 0) continue;
    thisGate -> setVar(v);
    //cout << "thisGateQQ" << endl;
    if(DFSList[i] != 0) fullList[DFSList[i]] += 2;
    if(thisGate -> getTypeStr() != "AIG") continue;
    solver.addAigCNF(v,thisGate -> getFanIn(0) -> getVar(), thisGate -> isInv(0),thisGate -> getFanIn(1) -> getVar(), thisGate -> isInv(1));
    
    vector<int>* FECGroup = thisGate -> getFECgroup();
    if(FECGroup == 0) {
      //solver.addAigCNF(v,thisGate -> getFanIn(0) -> getVar(), thisGate -> isInv(0),thisGate -> getFanIn(1) -> getVar(), thisGate -> isInv(1));
      continue;
    }
    for(size_t j = 0; j < FECGroup -> size(); ++j){
      //cout <<  "in loop" << endl;
      if((*FECGroup)[j]/2 != thisGate -> getID() && fullList[(*FECGroup)[j]/2] % 4 > 1){
        mergeGate  =  getGate((*FECGroup)[j]/2);
        if(mergeGate == 0) {
          continue;
        }
        if(mergeGate == _const0){
          isFECInv = (thisGate -> isFECInv()) ^ ((*FECGroup)[j]%2);
          if(!isFECInv){
            if((thisGate -> getFanIn(0) == _const0 && !thisGate -> isInv(0)) || (thisGate -> getFanIn(1) == _const0 && !thisGate -> isInv(1)) || (thisGate -> getFanIn(0) == thisGate -> getFanIn(1) && thisGate -> isInv(0) != thisGate -> isInv(1))){
              //cout << "Fraig: " << mergeGate -> getID() << " merging ";
              //if(isFECInv) cout << "!" ;
             // cout << thisGate -> getID() << "...\n";
              --AIGNum;
              for(size_t k = 0; k < thisGate -> getFanInSize(); ++k){
                thisGate -> getFanIn(k) -> deleteFanOut(thisGate);
              }
              for(size_t k = 0; k < thisGate ->  getFanOutSize(); ++k){
                thisGate -> getFanOut(k) -> replaceFanIn(thisGate, (size_t)mergeGate + (thisGate -> isOutInv(k) ));
                mergeGate -> pushFanOut((size_t)thisGate -> getFanOut(k) + thisGate -> isOutInv(k) );  
              } 
              fullList[thisGate -> getID()] = 0;
              delete thisGate;
              break;
            }
            /*else if(thisGate -> getFanIn(0) == thisGate -> getFanIn(1) && thisGate -> isInv(0) != thisGate -> isInv(1)){
              cout << "Fraig: " << mergeGate -> getID() << " merging ";
             // if(isFECInv) cout << "!" ;
              cout << thisGate -> getID() << "...\n";
              --AIGNum;
              for(size_t k = 0; k < thisGate -> getFanInSize(); ++k){
                thisGate -> getFanIn(k) -> deleteFanOut(thisGate);
              }
              for(size_t k = 0; k < thisGate ->  getFanOutSize(); ++k){
                thisGate -> getFanOut(k) -> replaceFanIn(thisGate, (size_t)mergeGate + (thisGate -> isOutInv(k) ));
                mergeGate -> pushFanOut((size_t)thisGate -> getFanOut(k) + thisGate -> isOutInv(k));  
              }
              fullList[thisGate -> getID()] = 0;
              delete thisGate;
              break;
            }*/
           
            break;
          }
          else{
            if((thisGate -> getFanIn(0) == _const0 && thisGate -> isInv(0)) && (thisGate -> getFanIn(1) == _const0 && thisGate -> isInv(1))){
              /* << "Fraig: " << mergeGate -> getID() << " merging ";
              if(isFECInv) cout << "!" ;
              cout << thisGate -> getID() << "...\n";*/
              --AIGNum;
              for(size_t k = 0; k < thisGate -> getFanInSize(); ++k){
                thisGate -> getFanIn(k) -> deleteFanOut(thisGate);
              }
              for(size_t k = 0; k < thisGate ->  getFanOutSize(); ++k){
                thisGate -> getFanOut(k) -> replaceFanIn(thisGate, (size_t)mergeGate + (thisGate -> isOutInv(k) ^ isFECInv));
                mergeGate -> pushFanOut((size_t)thisGate -> getFanOut(k) + thisGate -> isOutInv(k) ^ isFECInv);  
              }       
              fullList[thisGate -> getID()] = 0;
              delete thisGate;
              break;
            }
           //solver.addAigCNF(v,thisGate -> getFanIn(0) -> getVar(), thisGate -> isInv(0),thisGate -> getFanIn(1) -> getVar(), thisGate -> isInv(1));
            break;
          }
        }
        //solver.addAigCNF(v,thisGate -> getFanIn(0) -> getVar(), thisGate -> isInv(0),thisGate -> getFanIn(1) -> getVar(), thisGate -> isInv(1));
        isFECInv = (thisGate -> isFECInv()) ^ ((*FECGroup)[j]%2);
        v = solver.newVar();
        solver.addXorCNF(v, thisGate -> getVar(), isFECInv, mergeGate -> getVar(), false);
        solver.assumeRelease();
        solver.assumeProperty(v,true);
        solver.assumeProperty(_const0 ->getVar(), false);
        //chrono::steady_clock::time_point startTime = chrono::steady_clock::now();
        result = solver.assumpSolve();
        if(result == 1){

         // cout << "SAT " << (*FECGroup)[j]/2 << " " << thisGate -> getID() << endl;
          for(size_t k = 0; k < PIList.size(); ++k){
            if(fullList[PIList[k]] % 4 > 1 && solver.getValue(getGate(PIList[k]) -> getVar())){
             // cout << PIList[k] << endl;
              getGate(PIList[k]) -> addSimValue(pow(2,SATCount));
           // cout << k << " " << getGate(PIList[k]) -> getSimValue() << endl;
            }
          }
         // cout  << "finish addSim" << endl;
          ++SATCount;
          if(SATCount == 64){
           //cout << "Sim" << endl;
            for(size_t k = 0; k < DFSList.size(); ++k){
              if(getGate(DFSList[k]) != 0)
                getGate(DFSList[k]) -> simulate();
            }
            collectSimGrp(true);
           // cout << "collectDone" << endl;
            //sortSimGrps();
           // cout << "sortDone" << endl;
            assignSimGrps(true);
            //cout << "Updating by SAT... Total #FEC group = " << simGrps.size() << "\n";
            SATCount = 0;
            initSimValue();
            break;
          }
          break;
        }
        else if(result == 0){
          /* cout<< "Fraig: " << mergeGate -> getID() << " merging ";
          if(isFECInv) cout << "!" ;
          cout << thisGate -> getID() << "...\n";*/
          --AIGNum;
          for(size_t k = 0; k < thisGate -> getFanInSize(); ++k){
            thisGate -> getFanIn(k) -> deleteFanOut(thisGate);
          }
          for(size_t k = 0; k < thisGate ->  getFanOutSize(); ++k){
            thisGate -> getFanOut(k) -> replaceFanIn(thisGate, (size_t)mergeGate + (thisGate -> isOutInv(k) ^ isFECInv));
            mergeGate -> pushFanOut((size_t)thisGate -> getFanOut(k) + thisGate -> isOutInv(k) ^ isFECInv);  
          }
          
          fullList[thisGate -> getID()] = 0;
          delete thisGate;
          break;
        }
       /* else {
          //cout << "hi" << endl;
          break;
        }*/
      }
      
      //if(j == FECGroup -> size() - 1) solver.addAigCNF(v,thisGate -> getFanIn(0) -> getVar(), thisGate -> isInv(0),thisGate -> getFanIn(1) -> getVar(), thisGate -> isInv(1));
    }
  }
  DFSList.resize(0);
  FloatList.resize(0);
  unUsedList.resize(0);
  for(size_t i = 0; i < fullList.size(); ++i){
    fullList[i] = fullList[i] & ~0x3;
   // cout << fullList[i] << endl;
  }
  BuildDFSList();
  buildFloat();
  resetSimGrps();
  
  solver.reset();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
