/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"


using namespace std;

extern CirMgr *cirMgr;

unsigned CirGate::globalRef = 1;

// TODO: Implement memeber functions for class(es) in cirGate.h

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
   string type = getTypeStr();
   string ID = to_string(getID());
   string symbol = getSymbol();
   int no = getLineNo();
   if(no == -1) no = 0;
   string lineNo = to_string(no);
  

   if(ID == "0"){
      type = "CONST0";
   }

   int lineWidth = type.length() + symbol.length() + ID.length() + lineNo.length() + 11;

   cout << "================================================================================" << endl;
   cout << "= " << type << "(" << ID << ")";
   if(symbol != "") {cout << "\"" <<  symbol <<  "\"" ; lineWidth += 2;}
   cout << ", line " << lineNo << endl;

   //cout << right << setw(50-lineWidth) << "=" << endl;
   cout << "= FECs:";
   vector<int>* v = (vector<int>*)(FECgroup & ~0x1);
   //cout << FECgroup << endl;
   if(v){
      for(size_t i = 0; i < v -> size(); ++i){
         if((*v)[i]/2 == getID()) continue;
         cout << " " ;
         if((*v)[i] % 2 != FECgroup %  2) cout << "!";
         cout << (*v)[i]/2;
      }
   }
   
   cout << endl;
   cout << "= Value: ";
   size_t s = getSimValue();
   bool b[64] = {0};
   int count = 0;
   while(s > 0) {
      b[count] = s % 2; 
      s/=2; 
      ++count;
   }
   for(int i = 63; i >= 0; --i){
      cout << b[i];
      if(i%8 == 0 && i != 0) cout  << "_";
   }
   cout  << endl;
   cout << "================================================================================" << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   string type = getTypeStr();
   int ID = getID();
   if(ID == 0){
      type = "CONST";
   }

   cout << type << " " << ID;
   if(level == 0) {
      cout << endl;
      return;
   }

   cout << endl;
   if(type == "UNDEF") return;
   CirGate* nextGate;
   bool checkSetGlobalRef = false;
   for(size_t i = 0; i < getFanInSize(); ++i){
      nextGate = getFanIn(i);
      cout << "  ";
      if(!checkSetGlobalRef){
         nextGate -> setGlobalRef();
         checkSetGlobalRef = true;
      }
      if(isInv(i)) cout << "!";
      nextGate -> DFSin(level - 1, level);
   } 
   
}

void
CirGate::DFSin(int level, int totalLevel){
   assert (level >= 0);
   string type = getTypeStr();
   int ID = getID();
   if(ID == 0){
      type = "CONST";
   }
   cout << type << " " << ID;
   if(level == 0 || type == "UNDEF") { 
      cout << endl;
      return;
   }
   if(isGlobalRef()) {
      cout << " (*)" << endl;
      return;
   }
   cout << endl;
  
  
   
   CirGate* nextGate;
   for(size_t i = 0; i < getFanInSize(); ++i){
      nextGate = getFanIn(i);
      for(int i = 0; i < totalLevel - level + 1; ++i) cout << "  ";
      if(isInv(i)) cout << "!";
      nextGate -> DFSin(level - 1, totalLevel);
      if(i == getFanInSize() - 1)  setToGlobalRef();
   } 
  
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   string type = getTypeStr();
   int ID = getID();
   if(ID == 0){
      type = "CONST";
   }

   cout << type << " " << ID;
   if(level == 0) {
      cout << endl;
      return;
   }

   cout << endl;
   if(type == "UNDEF") return;

   CirGate* nextGate;
   bool checkSetGlobalRef = false;
   for(size_t i = 0; i < getFanOutSize(); ++i){
      nextGate = getFanOut(i);
      cout << "  ";
      if(!checkSetGlobalRef){
         nextGate -> setGlobalRef();
         checkSetGlobalRef = true;
      }
      if(isOutInv(i)) cout << "!";
      nextGate -> DFSout(level - 1, level);
   } 
}

void
CirGate::DFSout(int level, int totalLevel){
   assert (level >= 0);
   string type = getTypeStr();
   int ID = getID();
   if(ID == 0){
      type = "CONST";
   }
   cout << type << " " << ID;
   if(level == 0) {
      cout << endl; 
      return;
   }
   if(isGlobalRef()) {
      cout << " (*)" << endl;
      return;
   }
   cout << endl;
  
   CirGate* nextGate;
   for(size_t i = 0; i < getFanOutSize(); ++i){
      nextGate = getFanOut(i);
      for(int i = 0; i < totalLevel - level + 1; ++i) cout << "  ";
      if(isOutInv(i)) cout << "!";
      nextGate -> DFSout(level - 1, totalLevel);
      if(i == getFanOutSize() - 1)  setToGlobalRef();
   } 
  
}

void
CirGate::simulate(){
   if(getTypeStr() == "PI"){
      if(getID() == 0) setSimValue(0);
      return;
   } 

   if(getTypeStr() == "PO"){
      if(!isInv(0)) setSimValue(getFanIn(0) -> getSimValue());
      else setSimValue(~(getFanIn(0) -> getSimValue()));
   }
      
   else{
      size_t sv0 = getFanIn(0) -> getSimValue();
      size_t sv1 = getFanIn(1) -> getSimValue();
      if(isInv(0)) sv0 = ~sv0;
      if(isInv(1)) sv1 = ~sv1;
      setSimValue(sv0 & sv1);
   }
      
   
}