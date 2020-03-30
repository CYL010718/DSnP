/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class CirGate
{
   #define NEG 0x1
public:
   CirGate() {}
   virtual ~CirGate() {}

   // Basic access methods
   const string& getTypeStr() const { return gateType; }
   void setTypeStr(const string& s) {gateType = s;}
   unsigned getLineNo() const { return lineNo; }

   // Printing functions
   virtual void printGate() const = 0;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;

   void DFSin(int,int);
   void DFSout(int,int);

   //
   virtual void setLineNo(int n) {lineNo = n;}
   virtual void setFanIn(size_t f, int n = 0){}
   virtual void pushFanOut(size_t out){};
   virtual void replaceFanIn(CirGate* targetPin, size_t newPin){}
  // virtual void replaceFanOut(size_t targetPin, size_t newPin){};
  
   int getID() const{return ID;}

   virtual CirGate* getFanIn(size_t n) const {return 0;}
   virtual CirGate* getFanOut(size_t n) const {return 0;}
   virtual size_t getFanInSize() const {return 0;}
   virtual size_t getFanOutSize() const {return 0;}
   
   void setSymbol(const string& str){symbol = str;}
   string getSymbol() const {return symbol;}
   virtual bool isInv(size_t n) const {return 0;}
   virtual bool isOutInv(size_t n) const {return 0;}

   void setToGlobalRef(){ref = globalRef;}
   bool isGlobalRef() const {return (ref == globalRef);}
   void setGlobalRef(){++globalRef;}

   virtual void deleteFanOut(CirGate* g){return;}

   virtual bool isAig() const { return false; }

   void resetSimValue() {simValue = 0;}
   void addSimValue(size_t n) {simValue += n;}
   void setSimValue(size_t n) {simValue = n;}
   size_t getSimValue() const {return simValue;}
   void simulate();
   void setFECgroup(size_t v) {FECgroup = v;}
   vector<int>* getFECgroup() const {return (vector<int>*)(FECgroup & ~0x1);}
   bool isFECInv() {return FECgroup % 2;}

   Var getVar() const {return var;}
   void setVar(const Var& v) {var = v;}

protected:
    int lineNo;
    int ID;
    static unsigned globalRef;
    unsigned ref;
    string gateType;
    string symbol;
    size_t simValue;
    size_t FECgroup;
    Var var;
};

class AIGGate: public CirGate{
  public:
    AIGGate() {
      CirGate::ref = 0;
      CirGate::symbol = "";
      fanIn_1 = 0;
      fanIn_2 = 0;
      lineNo = 0; 
      ID = 0;
      FECgroup = 0;
      simValue = 0;
      gateType = "UNDEF";
    }
    AIGGate(int lineno, int id){
      ref = 0;
      lineNo = lineno;
      ID = id;
      symbol = "";
      fanIn_1 = 0;
      fanIn_2 = 0;
      FECgroup = 0;
      simValue = 0;
      gateType = "UNDEF";
    }
    ~AIGGate(){}
    void setFanIn(size_t in, int n = 0) {if(n == 1) fanIn_1 = in; else fanIn_2 = in;}
    void pushFanOut(size_t out) {fanOut.push_back(out);}
    void replaceFanIn(CirGate* targetPin, size_t newPin){
       if(getFanIn(0) == targetPin) fanIn_1 = newPin;
       else fanIn_2 = newPin;
    }

    CirGate* getFanIn(size_t n) const {
      if(n == 0)return (CirGate*)(fanIn_1 & ~size_t(NEG));
      return (CirGate*)(fanIn_2 & ~size_t(NEG));
    }

    CirGate* getFanOut(size_t n) const {
      return (CirGate*)(fanOut[n] & ~size_t(NEG));
    }

    size_t getFanInSize() const {return 2;}
    size_t getFanOutSize() const {return fanOut.size();}
    bool isInv(size_t n) const {
      if(n == 0) return (fanIn_1 & NEG) ;
      return (fanIn_2 & NEG);
    }
    bool isOutInv(size_t n) const {return (fanOut[n] & NEG);}
    //string getTypeStr() const { if(lineNo == -1) return "UNDEF"; return "AIG";}
    void printGate() const {};

    void deleteFanOut(CirGate* g){
      for(size_t i = 0; i < fanOut.size(); ++i){
        if(getFanOut(i) == g) {
          fanOut.erase(fanOut.begin()+i);
          return;
        }
      }
      return;
    }

  private:
    size_t fanIn_1;
    size_t fanIn_2;
    vector<size_t> fanOut;
};

class PIGate: public CirGate{
  public:
    PIGate() {}
    PIGate(int lineno, int id) {
      CirGate::ref = 0;
      CirGate::lineNo = lineno;
      CirGate::ID = id;
      CirGate::symbol = "";
      FECgroup = 0;
      simValue = 0;
      gateType = "PI";
    }
    ~PIGate(){}
    void pushFanOut(size_t out) {fanOut.push_back(out);}
    //string getTypeStr() const { return "PI"; }
    void printGate() const {};
    CirGate* getFanOut(size_t n) const {
      return (CirGate*)(fanOut[n] & ~size_t(NEG));
    }
    size_t getFanOutSize() const {return fanOut.size();}
    bool isOutInv(size_t n) const {return (fanOut[n] & NEG);}

    void deleteFanOut(CirGate* g){
      //cout << g << " " << getFanOut(0) << getID() << endl;
      for(size_t i = 0; i < fanOut.size(); ++i){
        if(getFanOut(i) == g) {
          fanOut.erase(fanOut.begin()+i);
          return;
        }
      }
    }
   
  private:
    vector<size_t> fanOut;
};

class POGate: public CirGate{
 public:
    POGate() {}
    POGate(int lineno, int id){
      CirGate::ref = 0;
      CirGate::lineNo = lineno;
      CirGate::ID = id;
      CirGate::symbol = "";
      FECgroup = 0;
      simValue = 0;
      gateType = "PO";
    }
    ~POGate(){}
    void setFanIn(size_t in, int n) {
      fanIn = in;
    }
    void replaceFanIn(CirGate* target,size_t newPin){
      fanIn = newPin;
    }
    CirGate* getFanIn(size_t n) const {
      if(n > 0) return 0;
      return (CirGate*)(fanIn & ~size_t(NEG));
    }

    size_t getFanInSize() const {return 1;}

    bool isInv(size_t n) const {return (fanIn & NEG) ;}
    //string getTypeStr() const { return "PO"; }
    void printGate() const {};

    bool isAig(){};

  private:
    size_t fanIn;
};


#endif // CIR_GATE_H
