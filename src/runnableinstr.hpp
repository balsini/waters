/***************************************************************************
    begin                : Thu Apr 24 15:54:58 CEST 2003
    copyright            : (C) 2003 by Giuseppe Lipari
    email                : lipari@sssup.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
 * $Id: exeinstr.hpp,v 1.7 2005/10/05 16:46:22 cesare Exp $
 *
 * $Log: exeinstr.hpp,v $
 * Revision 1.7  2005/10/05 16:46:22  cesare
 * Added DLL support.
 *
 * Revision 1.6  2005/04/28 01:34:47  cesare
 * Moved to sstream. Headers install. Code cleaning.
 *
 * Revision 1.5  2004/11/26 03:47:10  cesare
 * Finished merging the main trunk with Lipari's branch.
 *
 */
#ifndef __RUNNABLEINSTR_HPP__
#define __RUNNABLEINSTR_HPP__

#include <memory>

//From metasim
#include <debugstream.hpp>
#include <event.hpp>
#include <randomvar.hpp>
#include <simul.hpp>

//From RTLIB
#include <instr.hpp>


#include "../examples/waters/shared.h"
#include "../examples/waters/weibullvar.h"

namespace RTSim {

  using namespace std;
  using namespace MetaSim;

  /**
      \ingroup instr

      These instructions (ExecInstr & FixedInstr) are used to model a
      basic block of time-consuming computation, and are the basic
      building block of a real task

      @author Luigi Palopoli, Giuseppe Lipari, Gerardo Lamastra, Antonio Casile
      @version 2.0
      @see Instr */

  class RunnableInstr : public Instr {
    protected:
      /// End of Instruction flag
      bool flag;
      /// Random var representing the instruction cost/duration
      auto_ptr<RandomVar> cost_generator;
      /// Actual Real-Time execution of the instruction
      Tick execdTime;
      Tick maximumcost;
      /// Duration of the current instruction
      Tick currentCost;
      /// Execution time spent by the instruction
      double actTime;
      /// Last instant of time this instruction was scheduled
      Tick lastTime;
      /// True if the instruction is currently executing
      bool executing;
    public:

      EndInstrEvt _endEvt;

      RunnableInstr(Task *f, const string &n = "");
      static Instr *createInstance(vector<string> &par);

      virtual ~RunnableInstr() {}

      //Virtual methods from Instr
      virtual void schedule() throw (InstrExc);
      virtual void deschedule();
      virtual void onEnd();
      virtual void reset();
      virtual Tick getDuration() const;
      virtual Tick getWCET() const throw(RandomVar::MaxException);
      virtual Tick getExecTime() const;
      virtual void setTrace(Trace *t);

      //From Entity...
      virtual void newRun();
      virtual void endRun();


      /** Function inherited from Instr. It refreshes the state of the
     *  executing instruction when a change of the CPU speed occurs.
     */
      void refreshExec(double oldSpeed, double newSpeed);

  };

} // namespace RTSim 

#endif
