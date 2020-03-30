/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class HashKey{
  public:
    HashKey(size_t k0, size_t k1){
      if(k0 > k1) swap(k0,k1);
      key_0 = k0;
      key_1 = k1;
    };
    ~HashKey(){}

    size_t getFirstKey() const {return key_0;}
    size_t getSecondKey() const {return key_1;}

    bool operator == (const HashKey& k) const {return (key_0 == k.key_0 && key_1 == k.key_1);}

  private:
    size_t key_0;
    size_t key_1;
};

class HashFunction{
  public:
    size_t operator()(const HashKey& k) const {
      return ((hash<size_t>()(k.getFirstKey()) >> 1) ^ (hash<size_t>()(k.getSecondKey()) << 1));
    }
};

class CirMgr
{
   #define isDFS 0x3
public:
   CirMgr() {}
   ~CirMgr() {
     reset();
   }
   
   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const { 
     if(fullList[gid] == 0) return 0;
     return (CirGate*)(fullList[gid]& ~isDFS);
    }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() /*const*/;
   void writeAag(ostream&) const;

   //
   void initFullList(size_t size) {
     PIList.reserve(InputNum);
     POList.reserve(OutputNum);
     DFSList.reserve(size);
     FloatList.reserve(size);
     unUsedList.reserve(size);
     //vector<size_t> newVector;
     //newVector.assign(1,0);
     fullList.reserve(size);
     fullList.assign(size,0); 
     fullList[0] = (size_t)_const0;
     simGrps.reserve(size);
    }
   void pushPI(size_t gate) {PIList.push_back(gate);}
   void pushPO(size_t gate) {POList.push_back(gate);}
   void setFull(int ID, CirGate* gate) {fullList[ID] = (size_t)gate;}
 //  void setUndefine(int ID, size_t gate) {fullList[ID].push_back(gate);}
  // bool isUndefine(int ID) {return (fullList[ID].size() != 1);}
   void deleteUndefine(int, CirGate*);
   
   //
   void BuildDFSList();
   void DFS(CirGate*) ;

   void inputHeader(fstream&);
   void inputPI(fstream&);
   void inputPO(fstream&);
   void inputAIG(fstream&);
   void inputSymbol(fstream&);
   void inputComment(fstream&);
   void buildFloat();

   void reset(){
     for(size_t i = 1; i < fullList.size(); ++i){
       if(fullList[i] != 0) {
         delete getGate(i);
       }
     }
   }

   // Member functions about circuit optimization
   void sweep();
   void DFSweep(CirGate* );
   void optimize();

   // Member functions about simulation
   void randomSim() ;
   void initSimValue();
   void fileSim(ifstream&);
   void initSimGrps();
   int collectSimGrp(bool);
   void sortSimGrps();
   void assignSimGrps(bool);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }
   void outputSim(int);

   // Member functions about fraig
   void strash() ;
   void printFEC() const ;
   void fraig();
   void resetSimGrps();

   void printFECPairs() const;
   void writeGate(ostream&, CirGate*) const;

private:
   int MaxVariable;
   int InputNum;
   int LatchNum = 0;
   int OutputNum;
   int AIGNum;
   vector<int> PIList;
   vector<int> POList;

   vector<size_t> fullList;

   vector<int> DFSList;
   vector<int> FloatList;
   vector<int> unUsedList;

   static CirGate* _const0;
   unordered_map <HashKey, CirGate*, HashFunction> hashMap;
   vector<vector<int>*> simGrps;
   unordered_map<size_t, vector<int>*> grpHash;
   ofstream           *_simLog;

   

};


#endif // CIR_MGR_H
