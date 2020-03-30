/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;



static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine constant (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/

CirGate* CirMgr:: _const0 = new PIGate(0,0);

bool
CirMgr::readCircuit(const string& fileName)
{
   fstream file;
   file.open(fileName);
   if(!file) {
      cerr << "Cannot open design \"" << fileName << "\"!!" << endl;
      return false;
   }

   inputHeader(file);
   
   inputPI(file);
  // cout << "PO" << endl;
   inputPO(file);
 // cout << "AIG" << endl;
   inputAIG(file);
 // cout << "connect" << endl;
   buildFloat();
  //cout << "sym   " << endl;
   inputSymbol(file);
   //cout << "DFS" << endl;
   BuildDFSList();
   
  
   
   
   return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl;
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   cout << "  PI" << right << setw(12) << InputNum << endl;
   cout << "  PO" << right << setw(12) << OutputNum << endl;
   cout << "  AIG" << right << setw(11) << AIGNum << endl;
   cout << "------------------" << endl;
   cout << "  Total" << right << setw(9) << InputNum + OutputNum + AIGNum << endl;
}

void
CirMgr::printNetlist() const
{
   CirGate* tmpGate;
   CirGate* gate;
   cout << endl;
   for(size_t i = 0; i < DFSList.size(); ++i){
      gate = getGate(DFSList[i]);
      //cout  << gate << " " << DFSList[i] << endl;
      cout << "[" << i << "]" << " ";
      if(DFSList[i] == 0) {
         cout << "CONST0" << endl;
         continue;
      }
      cout << setw(3) << left << gate -> getTypeStr();
      cout << " " << DFSList[i];
      for(size_t j = 0; j < gate -> getFanInSize(); ++j){
         tmpGate = gate -> getFanIn(j);
         cout << " ";
         if(tmpGate -> getTypeStr() == "UNDEF") cout << "*";
         if(gate -> isInv(j)) cout << "!";
         cout << tmpGate -> getID();
      }
      if(gate -> getSymbol() != "") cout << " (" << gate -> getSymbol() << ")";
      cout << endl;

   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for(int i = 0; i < InputNum; ++i){
      cout << " " << PIList[i];
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for(int i = 0; i < OutputNum; ++i){
      cout << " " << POList[i];
   }
   cout << endl;
}

void
CirMgr::printFloatGates() /*const*/
{
   if(FloatList.size() != 0){
      cout << "Gates with floating fanin(s):";
      for(size_t i = 0; i < FloatList.size(); ++i){
         cout << " " << FloatList[i];
      }
      cout << endl;
   }

   if(unUsedList.size() != 0){
      cout << "Gates defined but not used  :" ;
      for(size_t i = 0; i < unUsedList.size();++i){
         cout << " " << unUsedList[i];
      }
      cout << endl;
   }

}

void
CirMgr::writeAag(ostream& outfile) const
{
   CirGate* currentGate;
   CirGate* nextGate;
   int ID;
   int AIGCounter = 0;

   outfile << "aag " << MaxVariable << " " << InputNum << " " << LatchNum << " " << OutputNum << " ";
  
   for(size_t i = 0; i < DFSList.size(); ++i){
      currentGate = getGate(DFSList[i]);
      if(currentGate -> getTypeStr() == "AIG") ++AIGCounter;
   }
   outfile << AIGCounter << endl;
   
   for(size_t i = 0; i < PIList.size(); ++i){
      outfile << PIList[i]*2 << endl;
   }

   
   for(size_t i = 0; i < POList.size(); ++i){
      currentGate = getGate(POList[i]);
      nextGate = currentGate -> getFanIn(0);
      if(currentGate -> isInv(0)) outfile << (nextGate -> getID())*2 + 1 << endl;
      else outfile << (nextGate -> getID())*2 << endl;
   }

   for(size_t i = 0; i < DFSList.size(); ++i){
      currentGate = getGate(DFSList[i]);
      if(currentGate -> getTypeStr() != "AIG") continue;
      outfile << (currentGate -> getID())*2;
      for(size_t j = 0; j < currentGate -> getFanInSize() ; ++j){
         nextGate = currentGate -> getFanIn(j);
         
         ID = (nextGate -> getID())*2;
         if(currentGate -> isInv(j)) ++ID;
         
         outfile << " " << ID;
      }
      outfile << endl;
   }

   for(size_t i = 0; i < PIList.size(); ++i){
      currentGate = getGate(PIList[i]);
      if(currentGate -> getSymbol() != ""){
         outfile << "i" << i << " " << currentGate -> getSymbol() << endl;
      }
   }

   for(size_t i = 0; i < POList.size(); ++i){
      currentGate = getGate(POList[i]);
      if(currentGate -> getSymbol() != ""){
         outfile << "o" << i << " " << currentGate -> getSymbol() << endl;
      }
   }
}
/*
void 
CirMgr::deleteUndefine(int ID, CirGate* newGate) {
   for(size_t i = 1; i < fullList[ID].size(); ++i){ 
      newGate -> pushFanOut(fullList[ID][i]);
   }
}*/

void 
CirMgr::inputHeader(fstream& file){
   string s;
     file >> s;
     file >> MaxVariable >> InputNum >> LatchNum >> OutputNum >> AIGNum;
     initFullList(MaxVariable + OutputNum + 1);
}

 void 
 CirMgr::inputPI(fstream& file){
      int pin1;
      for(int i = 0; i < InputNum; ++i){
          file >> pin1;
          CirGate* newPI =  new PIGate(i+2, pin1/2);
          pushPI(pin1/2);
          
          setFull(pin1/2, newPI);
          //cout << "hi" << endl;
      }
   }

void 
CirMgr::inputPO(fstream& file){
   int pin1;
   CirGate* newPO;
   CirGate* pin1AIG;
   for(int i = 0; i < OutputNum; ++i){
      file >> pin1;
      newPO = new POGate(i+InputNum+2, MaxVariable + 1 + i);
      pin1AIG = getGate(pin1/2);

      if(!pin1AIG) {
         pin1AIG = new AIGGate(-1, pin1 / 2);
         setFull(pin1/2, pin1AIG);
      }

      pushPO(MaxVariable + 1 + i);
      setFull(MaxVariable + 1 + i, newPO);

      newPO -> setFanIn((size_t)pin1AIG + pin1 % 2);
      pin1AIG -> pushFanOut((size_t)newPO + pin1 % 2);
   }
}

void 
CirMgr::inputAIG(fstream& file){
   int pin1,pin2;
   int gate;
   CirGate* pin1AIG;
   CirGate* pin2AIG;
   CirGate* newGate;
   for(int i = 0; i < AIGNum; ++i){
         file >> gate >> pin1 >> pin2;
         pin1AIG = getGate(pin1/2);
         newGate = getGate(gate/2);

         if(!newGate) {
            newGate = new AIGGate(i + InputNum + OutputNum + 2, gate/2);
            setFull(gate/2, newGate);
            newGate -> setTypeStr("AIG");
         }
         else {
            newGate -> setLineNo(i + InputNum + OutputNum + 2);
            newGate -> setTypeStr("AIG");
         }
        
         if(!pin1AIG){
            pin1AIG = new AIGGate(-1, pin1 / 2);
            setFull(pin1/2, pin1AIG);
         }

         pin2AIG = getGate(pin2/2);
         if(!pin2AIG){
            pin2AIG = new AIGGate(-1, pin2/2);
            setFull(pin2/2, pin2AIG);
         }

         newGate -> setFanIn((size_t)pin1AIG + pin1%2,1);
         newGate -> setFanIn((size_t)pin2AIG + pin2%2,2);

         
         pin1AIG -> pushFanOut((size_t)newGate + pin1 % 2);
         pin2AIG -> pushFanOut((size_t)newGate + pin2 % 2);
         
         
   }
}

void
CirMgr::inputSymbol(fstream& file){
   string gateName;
   string symbol;
   int gateNo;
   CirGate* thisGate;

   while(file >> gateName >> symbol && gateName != "c"){
      //cout << gateName << endl;
      if(gateName[0] == 'i'){
         gateName = gateName.substr(1);
         myStr2Int(gateName,gateNo);
         thisGate = getGate(PIList[gateNo]);
         thisGate -> setSymbol(symbol); 
      }
      else if(gateName[0] == 'o'){
         gateName = gateName.substr(1);
         myStr2Int(gateName,gateNo);
         thisGate = getGate(POList[gateNo]);
         thisGate -> setSymbol(symbol); 
      }
   }
}

void
CirMgr::inputComment(fstream& file){

}

void
CirMgr::BuildDFSList(){
   _const0 -> setGlobalRef();
   for(size_t i = 0; i < POList.size(); ++i){
      DFS(getGate(POList[i]));
   }
}

void
CirMgr::DFS(CirGate* gate){
   CirGate* nextGate;
   //cout << gate << " " << gate -> getID() << endl;
  
   for(size_t i = 0; i < gate -> getFanInSize(); ++i){
      nextGate = gate ->  getFanIn(i) ;
      //cout << "nextGate" << nextGate << endl;
      if(nextGate != 0 && !nextGate -> isGlobalRef() && !(nextGate -> getTypeStr() == "UNDEF")) {
         DFS(nextGate);
         nextGate -> setToGlobalRef();
      }
   }  
   DFSList.push_back(gate -> getID());
   if(!(fullList[gate -> getID()] & isDFS)) ++fullList[gate -> getID()];
}


void
CirMgr::buildFloat(){
   CirGate* thisGate;
   CirGate* nextGate;
   for(size_t i = 0; i < fullList.size(); ++i){
      thisGate = getGate(i);
      if(thisGate == 0 || thisGate -> getTypeStr() == "UNDEF") continue;

      for(size_t j = 0; j < thisGate -> getFanInSize(); ++j){
         nextGate = thisGate -> getFanIn(j);
         if(nextGate -> getTypeStr() == "UNDEF") {
            FloatList.push_back(i);
            break;
         }
      }

      if(thisGate -> getFanOutSize() == 0 && thisGate -> getTypeStr() != "PO" && i != 0) unUsedList.push_back(i);
   }
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
}

void
CirMgr::printFECPairs() const
{

   for(size_t i = 0; i < simGrps.size(); ++i){
      cout << "[" << i << "]";
      cout << " " << (*simGrps[i])[0] / 2;
      for(size_t j = 1; j < simGrps[i] -> size(); ++j){
         cout << " ";
         if((*simGrps[i])[j] % 2 == 1) cout << "!";
         cout << (*simGrps[i])[j] / 2;
    
      }
      cout << endl;
   }
}
