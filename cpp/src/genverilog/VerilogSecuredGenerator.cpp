/**
 * VerilogSecuredGenerator.cpp
 *
 * This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send 
 * a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.  
 *
 */

// Includes
//------------------

//-- Std
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdlib>
using namespace std;

//-- Qt
#include <QtCore>

//-- Core
#include <core/Fsm.h>
#include <core/State.h>
#include <core/Trans.h>
#include <core/Hypertrans.h>
#include <core/Condition.h>

//-- Verification
#include <verification/verify.h>
#include <verification/logicmin.h>
#include <verification/invertDNF.h>


#include "VerilogSecuredGenerator.h"

VerilogSecuredGenerator::VerilogSecuredGenerator() {
    this->Statecov = true;
    this->Transcov = true;

}

VerilogSecuredGenerator::~VerilogSecuredGenerator() {
}

Generator * VerilogSecuredGenerator::newInstance() {

    return new VerilogSecuredGenerator();

}


QString VerilogSecuredGenerator::getName()  {

    return "Verilog Generator";
}


QString VerilogSecuredGenerator::getDescription()  {

    return "Verilog Generator from CAG";

}

void VerilogSecuredGenerator::generate(Fsm * fsm, QDataStream * dataStream) {

    //-- Open Target file
    QTextStream out (dataStream->device());

    int numberofinputs = fsm->getNumberOfInputs();
    int numberofoutputs = fsm->getNumberOfOutputs();
    int numberofoutputsHamming = 0;
    int resetstate = fsm->getResetState();
    int tmrcount = 1;

    if (numberofoutputs == 1) {
    	numberofoutputsHamming = 3;
	} else if (numberofoutputs < 4) {
    	numberofoutputsHamming = 7;
	} else if (numberofoutputs < 11) {
    	numberofoutputsHamming = 15;
	} else if (numberofoutputs < 26) {
    	numberofoutputsHamming = 31;
	} else if (numberofoutputs <= 57) {
    	numberofoutputsHamming = 63;
	} else {
		cout << "Error: No possible count of bits";
		exit(0);
	}

    //calcHammingDistance();
    //extendStateEncoding();
    //createStateVariations();

    // Write license
    //-----------------
    out << "/**" << endl;

    //FIXME  Write Instance pattern
    //----------------------------
    out << " " << this->createInstance(fsm) << endl;
    out << " */" << endl;

    // Write includes
    //----------------------
    if (this->getParameter("includes").isValid()) {
        QStringList includes = this->getParameter("includes").toStringList();
        for (int i = 0; i < includes.size(); i++) {
            out << "`include \""
                    << includes.at(i) << "\""
                    << endl;
        }
    }


    // Write module header (I/O)
    //----------------------------
    out << "module " << fsm->getFsmName().c_str() << " ( " << endl;
    out << "    input wire " << fsm->getClockName().c_str() << ", "
            << endl;
    out << "    input wire " << fsm->getResetName().c_str() << ", "
            << endl << endl;

    //---- Input
    out << "    // Inputs" << endl << "    //------------ " << endl;
    for (int i = 0; i < numberofinputs; i++) {
        out << "    input wire " << fsm->getInputName(i).c_str() << ", "
                << endl;
    }
    out << endl;

    //---- Outputs
    out << "    // Outputs" << endl << "    //------------ " << endl;
    for (int i = 0; i < numberofoutputs - 1; i++) {
        out << "    output wire " << fsm->getOutputName(i).c_str() << ", "
                << endl;
    }
    out << "    output wire "
            << fsm->getOutputName(numberofoutputs - 1).c_str();

    //---- Forward declaration?
    if (this->getParameter("forward.async").toBool()) {
        out << ", " << endl << endl;
        out << "    //-- Async Forward state" << endl;
        out << "    output wire [" << (numberofoutputs - 1)
                << ":0] forward_state_async";

    }
    if (this->getParameter("forward.delayed").toBool()) {
        out << ", " << endl << endl;
        out << "    //-- Delayed Forward state" << endl;
        out << "    output reg [" << (numberofoutputs - 1)
                << ":0] forward_state_delayed ";

    }
    if (this->getParameter("forward.sync").toBool()) {
        out << ", " << endl << endl;
        out << "    //-- Forwarded state (Actual State)" << endl;
        out << "    output wire [" << (numberofoutputs - 1)
                << ":0] forward_state ";
    }
    out << endl;

    out << " );" << endl;
    out << endl;

    // Write Parameters (States)
    //-----------------------------
    FOREACH_STATE(fsm)

    	out << "localparam " << state->getName().c_str() << " = "
				<< numberofoutputs << "'b" << state->getOutput().c_str() << ";"
				<< endl;

    	if (this->getParameter("Hamming").toBool()) {
    		state->setHammingOutput();
    		cout << "State without / with Hamming-Distance of 3: " << state->getOutput().c_str() << " / " << state->getOutputHamming().c_str() << endl;
    		out << "localparam " << state->getName().c_str() << "_Hamming = "
    				<< numberofoutputsHamming << "'b" << state->getOutputHamming().c_str() << ";"
    				<< endl;
    	}

    END_FOREACH_STATE

    // current_state Declaration
    //----------------------------------
    if (this->getParameter("TMR").toBool()) {
        out << endl << "reg [" << (numberofoutputs - 1)
                << ":0] current_state, next_state, next_state_hamm, next_state_1, next_state_2, next_state_3;" << endl;
    } else {
        out << endl << "reg [" << (numberofoutputs - 1)
                << ":0] current_state, next_state, next_state_hamm;" << endl;
    }


    out << "assign {";
    for (int i = 0; i < numberofoutputs - 1; i++) {
        out << fsm->getOutputName(i).c_str() << ", ";
    }
    out << fsm->getOutputName(numberofoutputs - 1).c_str();
    out << "} = current_state;" << endl;

    //-- If Async Forward declaration, forward_state_async is the next_state
    if (this->getParameter("forward.async").toBool()) {
        out << "assign forward_state_async = next_state;" << endl;
    }
    //-- If normal forward, forward_state is current_state
    if (this->getParameter("forward.sync").toBool()) {
        out << "assign forward_state = current_state;" << endl;
    }
    out << endl;

    // input vector declaration
    //----------------------
    out << "wire [" << (numberofinputs - 1) << ":0] inputvector;"
            << endl;
    out << "assign inputvector = {";
    for (int i = 0; i < numberofinputs - 1; i++) {
        out << fsm->getInputName(i).c_str() << ", ";
    }
    out << fsm->getInputName(numberofinputs - 1).c_str();
    out << "};" << endl << endl << endl;

    // Write combinational logic always-block
    //--------------------------------------
    out << "always @(*) begin" << endl;

    if (this->getParameter("TMR").toBool()) {
        cout << "TMR active" << endl;
        tmrcount = 3;
    } else {
        cout << "TMR not active" << endl;
        tmrcount = 1;
    }

    int tmri;

    for(tmri=1; tmri <= tmrcount; tmri++) {

        cout << "Working on case-definition " << tmri << " of " << tmrcount << endl;

    	//-- If we are in delayed forward mode, use the forward_state declaration for computing
		if (this->getParameter("forward.delayed").toBool()) {
			out << "  casex({inputvector, forward_state_delayed})" << endl;
		} else {
			out << "  casex({inputvector, current_state})" << endl;
		}

		// Foreach State
		//------------------------
		FOREACH_STATE(fsm)

			cout << "Working on State " << state->getName() << endl;

			//---- Foreach Transitions that start from this state
			//-----------------
			vector<string> input;

			//FOREACH_TRANSITIONS(fsm)

			// Determine Inputs for the transitions starting from this state, and non default
			//  - The conditions' inputs are then inverted, then minimized to determine all the cases matching the default transition
			//---------------------
			State * targetDefaultState = NULL;
			FOREACH_STATE_STARTING_TRANSITIONS(state)

				//-- Record default transition target
				targetDefaultState = transition->isDefault() ? transition->getEndState() : targetDefaultState;

				//-- Non default transition input is stacked for minimization
				if (!transition->isDefault()) {

					//-- Foreach Conditions
					FOREACH_TRANSITION_CONDITIONS(transition)

						string conditionInput =
								condition->getInput();
						string result = "";
						char X;
						for (int i = 0;
								i
										< fsm->getNumberOfInputs();
								i++) {
							X = conditionInput[i];
							if (X == 'x' || X == 'X') {
								result = result + '-';
							} else {
								result = result + X;
							}
						}
						input.push_back(result);

					END_FOREACH_TRANSITION_CONDITIONS


				}
			END_FOREACH_STATE_STARTING_TRANSITIONS

			// Determine input for hypertrans
			//---------------------
			FOREACH_HYPERTRANSITIONS(fsm)

				FOREACH_HYPERTRANSITION_CONDITIONS(hypertransition)

					string conditionInput =
							condition->getInput();
					string result = "";
					char X;
					for (int i = 0;
							i < fsm->getNumberOfInputs(); i++) {
						X = conditionInput[i];
						if (X == 'x' || X == 'X') {
							result = result + '-';
						} else {
							result = result + X;
						}
					}
					input.push_back(result);

				END_FOREACH_HYPERTRANSITION_CONDITIONS
			END_FOREACH_HYPERTRANSITIONS

			// Minimize input vector to find out bit conditions leading to default transition
			//-------------------------
			bool notfound = true;
			if (input.size() > 0) {

				//--
				InvertDNF INV(this->getParameter("removeIntersections").toBool());
				string defaultvalue = INV.invert(input);
				//string defaultvalue = input[0];
				cout << "\tDefault value: " << defaultvalue
						<< endl;
				int maxlen = defaultvalue.length();
				int len = maxlen;
				if (len > 0) {

					out << "    {" << fsm->getNumberOfInputs() << "'b";

					while (len > fsm->getNumberOfInputs()) {
						string s = defaultvalue;
						do {
							len--;
						} while ((s[len] != '+') && (len != 0));
						s.assign(s, (len + 1),
								(maxlen - len - 1));
						defaultvalue.assign(defaultvalue, 0,
								len);
						char X;
						for (int i = 0; i < fsm->getNumberOfInputs();
								i++) {
							X = s[i];
							if (X == '-' || X == 'X') {
								out << "x";
							} else {
								out << X;
							}
						}
						out
								<< ", "
								<< state->getName().c_str()
								<< "}," << endl;
						out << "    {" << fsm->getNumberOfInputs()
								<< "'b";
					}

					char X;
					for (int i = 0; i < fsm->getNumberOfInputs(); i++) {

						X = defaultvalue[i];
						if (X == '-' || X == 'X') {
							out << "x";
						} else {
							out << X;
						}
					}

					out
							<< ", "
							<< state->getName().c_str()
							<< "}";


					// End of default conditions
					// Write end case
					//-------------------
				    if (this->getParameter("TMR").toBool()) {
				    	if (this->getParameter("Hamming").toBool()) {
							out  << ":   next_state_" << tmri << " = "
									<< this->cleanString(targetDefaultState->getName()) << "_Hamming;"
									<< endl;
				    	} else {
							out  << ":   next_state_" << tmri << " = "
									<< this->cleanString(targetDefaultState->getName()) << ";"
									<< endl;
				    	}
				    } else {
				    	if (this->getParameter("Hamming").toBool()) {
							out  << ":   next_state = "
									<< this->cleanString(targetDefaultState->getName()) << "_Hamming;"
									<< endl;
				    	} else {
							out  << ":   next_state = "
									<< this->cleanString(targetDefaultState->getName()) << ";"
									<< endl;
				    	}
				    }

				}
			} else {


				out << "    {" << fsm->getNumberOfInputs() << "'b";
				string invec = "";
				for (int i = 0; i < fsm->getNumberOfInputs(); i++) {
					invec = invec + "x";
				}

				if (this->getParameter("TMR").toBool()) {
					if (this->getParameter("Hamming").toBool()) {
						out << invec.c_str() << ", " << state->getName().c_str()
								<< "}:   next_state_" << tmri << " = "
								<< this->cleanString(targetDefaultState->getName())
								<< "_Hamming;" << endl;
					} else {
						out << invec.c_str() << ", " << state->getName().c_str()
								<< "}:   next_state_" << tmri << " = "
								<< this->cleanString(targetDefaultState->getName())
								<< ";" << endl;
					}
				} else {
					if (this->getParameter("Hamming").toBool()) {
						out << invec.c_str() << ", " << state->getName().c_str()
								<< "}:   next_state = "
								<< this->cleanString(targetDefaultState->getName())
								<< "_Hamming;" << endl;
					} else {
						out << invec.c_str() << ", " << state->getName().c_str()
								<< "}:   next_state = "
								<< this->cleanString(targetDefaultState->getName())
								<< ";" << endl;
					}
				}


			}

			// Write out non default transitions
			//---------------------------
			FOREACH_STATE_STARTING_TRANSITIONS(state)
				State * transEndState = transition->getEndState();
				if(!transition->isDefault()) {

					// Foreach conditions and write out transition
					//--------------
					FOREACH_TRANSITION_CONDITIONS(transition)

						out << "    {" << fsm->getNumberOfInputs() << "'b";
						string conditionInput = condition->getInput();
						char X;
						for (int i = 0; i < fsm->getNumberOfInputs(); i++) {
							X = conditionInput[i];
							if (X == '-' || X == 'X') {
								out << "x";
							} else {
								out << X;
							}
						}

						if (this->getParameter("TMR").toBool()) {
							if (this->getParameter("Hamming").toBool()) {
								out << ", " << state->getName().c_str()
										<< "}:   next_state_" << tmri << " = "
										<< this->cleanString(transEndState->getName())
										<< "_Hamming;" << endl;
							} else {
								out << ", " << state->getName().c_str()
										<< "}:   next_state_" << tmri << " = "
										<< this->cleanString(transEndState->getName())
										<< ";" << endl;
							}
						} else {
							if (this->getParameter("Hamming").toBool()) {
								out << ", " << state->getName().c_str()
										<< "}:   next_state = "
										<< this->cleanString(transEndState->getName())
										<< "_Hamming;" << endl;
							} else {
								out << ", " << state->getName().c_str()
										<< "}:   next_state = "
										<< this->cleanString(transEndState->getName())
										<< ";" << endl;
							}
						}

					END_FOREACH_TRANSITION_CONDITIONS
				}
			END_FOREACH_STATE_STARTING_TRANSITIONS

		END_FOREACH_STATE
		// EOF States transitions //

		// FOREACH Hyper transitions
		//----------------------------------
		FOREACH_HYPERTRANSITIONS(fsm)

			//-- Foreach conditions
			FOREACH_HYPERTRANSITION_CONDITIONS(hypertransition)

					out << "    {" << fsm->getNumberOfInputs() << "'b";
					string conditionInput = condition->getInput();
					char X;
					for (int i = 0; i < fsm->getNumberOfInputs(); i++) {
						X = conditionInput[i];
						if (X == '-' || X == 'X') {
							out << "x";
						} else {
							out << X;
						}
					}
					out << ", " << fsm->getNumberOfOutputs() << "'b";
					for (int i = 0; i < fsm->getNumberOfOutputs(); i++) {
						out << "x";
					}

					if (this->getParameter("TMR").toBool()) {
						if (this->getParameter("Hamming").toBool()) {
							out << "}:   next_state_" << tmri << " = "
									<< this->cleanString(hypertransition->getTargetState()->getName())
									<< "_Hamming;" << endl;
						} else {
							out << "}:   next_state_" << tmri << " = "
									<< this->cleanString(hypertransition->getTargetState()->getName())
									<< ";" << endl;
						}
					} else {
						if (this->getParameter("Hamming").toBool()) {
							out << "}:   next_state = "
									<< this->cleanString(hypertransition->getTargetState()->getName())
									<< "_Hamming;" << endl;
						} else {
							out << "}:   next_state = "
									<< this->cleanString(hypertransition->getTargetState()->getName())
									<< ";" << endl;
						}
					}

			 END_FOREACH_HYPERTRANSITION_CONDITIONS

		 END_FOREACH_HYPERTRANSITIONS

		// Default Reset State
		// Not necessary if number of states is a power of 2 and
		// the number of output states is equal to number of outputs as a power of 2 !!!
		//------------------------
		int a;
		a = 1 << numberofoutputs;
		if (!(a == fsm->getStateCount())) {

		    if (this->getParameter("TMR").toBool()) {
				out << "    default:  next_state_" << tmri << " = ";
		    } else {
				out << "    default:  next_state = ";
		    }

		    if (this->getParameter("Hamming").toBool()) {
		    	out << this->cleanString(fsm->getStatebyID(fsm->getResetState())->getName()) << "_Hamming;" << endl;
		    } else {
		    	out << this->cleanString(fsm->getStatebyID(fsm->getResetState())->getName()) << ";" << endl;
		    }
		}

		out << "  endcase" << endl;

	}

    if (this->getParameter("TMR").toBool()) {
        if (this->getParameter("Hamming").toBool()) {
        	out << "  next_state_hamm = (next_state_1 & next_state_2) | (next_state_1 & next_state_3) | (next_state_2 & next_state_3);" << endl;
        } else {
        	out << "  next_state = (next_state_1 & next_state_2) | (next_state_1 & next_state_3) | (next_state_2 & next_state_3);" << endl;
        }
    }

    out << "end" << endl << endl;

    // Hamming-Decoder
    if (this->getParameter("Hamming").toBool()) {
    	out << "always @(*) begin" << endl;

    	switch (numberofoutputsHamming) {
			case 3:
			    if (!this->getParameter("HammingDirectOutput").toBool()) {
					out << "  // hamming decoder for 1 + 2 = 3 bits" << endl;

					/*
					 Berechnung der R-Bits

					 1 = 01
					 2 = 10
					 3 = 11

					r1: Ist das 0te bit gesetzt => 101
					r2: Ist das 1te bit gesetzt => 110

					*/

					out << "  r1 = ^(next_state_hamm & 3'b101);" << endl;
					out << "  r2 = ^(next_state_hamm & 3'b110);" << endl;
					out << "  r = {r2,r1};" << endl;
					out << "  IN = next_state_hamm;" << endl;
					out << "  IN[r-1] = ~IN[r-1];" << endl;
					out << "  i=0; j=0;" << endl << endl;
					out << "  while((i<n) || (j<k)) " << endl;
					out << "  begin" << endl;
					out << "    while(i==0 || i==1)" << endl;
					out << "      i=i+1;" << endl << endl;
					out << "    next_state[j]=IN[i];" << endl;
					out << "    i=i+1;" << endl;
					out << "    j=j+1;" << endl;
					out << "  end" << endl;
			    } else {
			    	out << "  // hamming direct output for 1 + 2 = 3 bits" << endl;
			    	out << "next_state[0] = next_state_Hamming[2];" << endl;
			    }
				break;
			case 7:
			    if (!this->getParameter("HammingDirectOutput").toBool()) {
					out << "  // hamming decoder for 4 + 3 = 7 bits" << endl;

					/*
					 Berechnung der R-Bits

					 1 = 001
					 2 = 010
					 3 = 011
					 4 = 100
					 5 = 101
					 6 = 110
					 7 = 111

					r1: Ist das 0te bit gesetzt => 1010101
					r2: Ist das 1te bit gesetzt => 1100110
					r4: Ist das 2te bit gesetzt => 1111000

					*/

					out << "  r1 = ^(next_state_hamm & 7'b1010101);" << endl;
					out << "  r2 = ^(next_state_hamm & 7'b1100110);" << endl;
					out << "  r4 = ^(next_state_hamm & 7'b1111000);" << endl;
					out << "  r = {r4,r2,r1};" << endl;
					out << "  IN = next_state_hamm;" << endl;
					out << "  IN[r-1] = ~IN[r-1];" << endl;
					out << "  i=0; j=0;" << endl << endl;
					out << "  while((i<n) || (j<k)) " << endl;
					out << "  begin" << endl;
					out << "    while(i==0 || i==1 || i==3)" << endl;
					out << "      i=i+1;" << endl << endl;
					out << "    next_state[j]=IN[i];" << endl;
					out << "    i=i+1;" << endl;
					out << "    j=j+1;" << endl;
					out << "  end" << endl;
			    } else {
			    	out << "  // hamming direct output for 4 + 3 = 7 bits" << endl;
			    	out << "next_state[0] = next_state_Hamming[2];" << endl;
			    	out << "next_state[1] = next_state_Hamming[4];" << endl;
			    	out << "next_state[2] = next_state_Hamming[5];" << endl;
			    	out << "next_state[3] = next_state_Hamming[6];" << endl;
			    }
				break;
			case 15:
			    if (!this->getParameter("HammingDirectOutput").toBool()) {
					out << "  // hamming decoder for 11 + 4 = 15 bits" << endl;

					/*
					 Berechnung der R-Bits

					 1 = 0001
					 2 = 0010
					 3 = 0011
					 4 = 0100
					 5 = 0101
					 6 = 0110
					 7 = 0111
					 8 = 1000
					 9 = 1001
					10 = 1010
					11 = 1011
					12 = 1100
					13 = 1101
					14 = 1110
					15 = 1111

					r1: Ist das 0te bit gesetzt => 101010101010101
					r2: Ist das 1te bit gesetzt => 110011001100110
					r4: Ist das 2te bit gesetzt => 111100001111000
					r8: Ist das 3te bit gesetzt => 111111110000000

					*/

					out << "  r1 = ^(next_state_hamm & 15'b101010101010101);" << endl;
					out << "  r2 = ^(next_state_hamm & 15'b110011001100110);" << endl;
					out << "  r4 = ^(next_state_hamm & 15'b111100001111000);" << endl;
					out << "  r8 = ^(next_state_hamm & 15'b111111110000000);" << endl;
					out << "  r = {r8,r4,r2,r1};" << endl;
					out << "  IN = next_state_hamm;" << endl;
					out << "  IN[r-1] = ~IN[r-1];" << endl;
					out << "  i=0; j=0;" << endl << endl;
					out << "  while((i<n) || (j<k)) " << endl;
					out << "  begin" << endl;
					out << "    while(i==0 || i==1 || i==3 || i==7)" << endl;
					out << "      i=i+1;" << endl << endl;
					out << "    next_state[j]=IN[i];" << endl;
					out << "    i=i+1;" << endl;
					out << "    j=j+1;" << endl;
					out << "  end" << endl;
			    } else {
					out << "  // hamming direct output for 11 + 4 = 15 bits" << endl;
			    	out << "next_state[0] = next_state_Hamming[2];" << endl;
			    	out << "next_state[1] = next_state_Hamming[4];" << endl;
			    	out << "next_state[2] = next_state_Hamming[5];" << endl;
			    	out << "next_state[3] = next_state_Hamming[6];" << endl;
			    	out << "next_state[4] = next_state_Hamming[8];" << endl;
			    	out << "next_state[5] = next_state_Hamming[9];" << endl;
			    	out << "next_state[6] = next_state_Hamming[10];" << endl;
			    	out << "next_state[7] = next_state_Hamming[11];" << endl;
			    	out << "next_state[8] = next_state_Hamming[12];" << endl;
			    	out << "next_state[9] = next_state_Hamming[13];" << endl;
			    	out << "next_state[10] = next_state_Hamming[14];" << endl;
			    }
				break;
			case 31:
			    if (!this->getParameter("HammingDirectOutput").toBool()) {
					out << "  // hamming decoder for 26 + 5 = 31 bits" << endl;

					/*
					 Berechnung der R-Bits

					 1 = 0001
					 2 = 0010
					 3 = 0011
					 4 = 0100
					 5 = 0101
					 6 = 0110
					 7 = 0111
					 8 = 1000
					 9 = 1001
					10 = 1010
					11 = 1011
					12 = 1100
					13 = 1101
					14 = 1110
					15 = 1111
					16 = 10000
					17 = 10001
					18 = 10010
					19 = 10011
					20 = 10100
					21 = 10101
					22 = 10110
					23 = 10111
					24 = 11000
					25 = 11001
					26 = 11010
					27 = 11011
					28 = 11100
					29 = 11101
					30 = 11110
					31 = 11111

					r1: Ist das 0te bit gesetzt =>  1010101010101010101010101010101
					r2: Ist das 1te bit gesetzt =>  1100110011001100110011001100110
					r4: Ist das 2te bit gesetzt =>  1111000011110000111100001111000
					r8: Ist das 3te bit gesetzt =>  1111111100000000111111110000000
					r16: Ist das 3te bit gesetzt => 1111111111111111000000000000000

					*/

					out << "  r1 = ^(next_state_hamm & 31'b1010101010101010101010101010101);" << endl;
					out << "  r2 = ^(next_state_hamm & 31'b1100110011001100110011001100110);" << endl;
					out << "  r4 = ^(next_state_hamm & 31'b1111000011110000111100001111000);" << endl;
					out << "  r8 = ^(next_state_hamm & 31'b1111111100000000111111110000000);" << endl;
					out << "  r16 = ^(next_state_hamm & 31'b1111111111111111000000000000000);" << endl;
					out << "  r = {r16,r8,r4,r2,r1};" << endl;
					out << "  IN = next_state_hamm;" << endl;
					out << "  IN[r-1] = ~IN[r-1];" << endl;
					out << "  i=0; j=0;" << endl << endl;
					out << "  while((i<n) || (j<k)) " << endl;
					out << "  begin" << endl;
					out << "    while(i==0 || i==1 || i==3 || i==7 || i ==15)" << endl;
					out << "      i=i+1;" << endl << endl;
					out << "    next_state[j]=IN[i];" << endl;
					out << "    i=i+1;" << endl;
					out << "    j=j+1;" << endl;
					out << "  end" << endl;
			    } else {
			    	out << "  // hamming direct output for 26 + 5 = 31 bits" << endl;
			    	out << "next_state[0] = next_state_Hamming[2];" << endl;
			    	out << "next_state[1] = next_state_Hamming[4];" << endl;
			    	out << "next_state[2] = next_state_Hamming[5];" << endl;
			    	out << "next_state[3] = next_state_Hamming[6];" << endl;
			    	out << "next_state[4] = next_state_Hamming[8];" << endl;
			    	out << "next_state[5] = next_state_Hamming[9];" << endl;
			    	out << "next_state[6] = next_state_Hamming[10];" << endl;
			    	out << "next_state[7] = next_state_Hamming[11];" << endl;
			    	out << "next_state[8] = next_state_Hamming[12];" << endl;
			    	out << "next_state[9] = next_state_Hamming[13];" << endl;
			    	out << "next_state[10] = next_state_Hamming[14];" << endl;
			    	out << "next_state[11] = next_state_Hamming[16];" << endl;
			    	out << "next_state[12] = next_state_Hamming[17];" << endl;
			    	out << "next_state[13] = next_state_Hamming[18];" << endl;
			    	out << "next_state[14] = next_state_Hamming[19];" << endl;
			    	out << "next_state[15] = next_state_Hamming[20];" << endl;
			    	out << "next_state[16] = next_state_Hamming[21];" << endl;
			    	out << "next_state[17] = next_state_Hamming[22];" << endl;
			    	out << "next_state[18] = next_state_Hamming[23];" << endl;
			    	out << "next_state[19] = next_state_Hamming[24];" << endl;
			    	out << "next_state[20] = next_state_Hamming[25];" << endl;
			    	out << "next_state[21] = next_state_Hamming[26];" << endl;
			    	out << "next_state[22] = next_state_Hamming[27];" << endl;
			    	out << "next_state[23] = next_state_Hamming[28];" << endl;
			    	out << "next_state[24] = next_state_Hamming[29];" << endl;
			    	out << "next_state[25] = next_state_Hamming[30];" << endl;
			    }
			    break;
			case 63:
			    if (!this->getParameter("HammingDirectOutput").toBool()) {
					out << "  // hamming decoder for 57 + 6 = 63 bits" << endl;

					/*
					 Berechnung der R-Bits

					 1 = 0001
					 2 = 0010
					 3 = 0011
					 4 = 0100
					 5 = 0101
					 6 = 0110
					 7 = 0111
					 8 = 1000
					 9 = 1001
					10 = 1010
					11 = 1011
					12 = 1100
					13 = 1101
					14 = 1110
					15 = 1111
					16 = 10000
					17 = 10001
					18 = 10010
					19 = 10011
					20 = 10100
					21 = 10101
					22 = 10110
					23 = 10111
					24 = 11000
					25 = 11001
					26 = 11010
					27 = 11011
					28 = 11100
					29 = 11101
					30 = 11110
					31 = 11111
					32 = 100000
					33 = 100001
					34 = 100010
					35 = 100011
					36 = 100100
					37 = 100101
					38 = 100110
					39 = 100111
					40 = 101000
					41 = 101001
					42 = 101010
					43 = 101011
					44 = 101100
					45 = 101101
					46 = 101110
					47 = 101111
					48 = 110000
					49 = 110001
					50 = 110010
					51 = 110011
					52 = 110100
					53 = 110101
					54 = 110110
					55 = 110111
					56 = 111000
					57 = 111001
					58 = 111010
					59 = 111011
					60 = 111100
					61 = 111101
					62 = 111110
					63 = 111111

					r1: Ist das 0te bit gesetzt =>  101010101010101010101010101010101010101010101010101010101010101
					r2: Ist das 1te bit gesetzt =>  110011001100110011001100110011001100110011001100110011001100110
					r4: Ist das 2te bit gesetzt =>  111100001111000011110000111100001111000011110000111100001111000
					r8: Ist das 3te bit gesetzt =>  111111110000000011111111000000001111111100000000111111110000000
					r16: Ist das 4te bit gesetzt => 111111111111111100000000000000001111111111111111000000000000000
					r32: Ist das 5te bit gesetzt => 111111111111111111111111111111110000000000000000000000000000000

					*/

					out << "  r1 = ^(next_state_hamm & 63'b101010101010101010101010101010101010101010101010101010101010101);" << endl;
					out << "  r2 = ^(next_state_hamm & 63'b110011001100110011001100110011001100110011001100110011001100110);" << endl;
					out << "  r4 = ^(next_state_hamm & 63'b111100001111000011110000111100001111000011110000111100001111000);" << endl;
					out << "  r8 = ^(next_state_hamm & 63'b111111110000000011111111000000001111111100000000111111110000000);" << endl;
					out << "  r16 = ^(next_state_hamm & 63'b111111111111111100000000000000001111111111111111000000000000000);" << endl;
					out << "  r32 = ^(next_state_hamm & 63'b111111111111111111111111111111110000000000000000000000000000000);" << endl;
					out << "  r = {r32,r16,r8,r4,r2,r1};" << endl;
					out << "  IN = next_state_hamm;" << endl;
					out << "  IN[r-1] = ~IN[r-1];" << endl;
					out << "  i=0; j=0;" << endl << endl;
					out << "  while((i<n) || (j<k)) " << endl;
					out << "  begin" << endl;
					out << "    while(i==0 || i==1 || i==3 || i==7 || i ==15 || i==31)" << endl;
					out << "      i=i+1;" << endl << endl;
					out << "    next_state[j]=IN[i];" << endl;
					out << "    i=i+1;" << endl;
					out << "    j=j+1;" << endl;
					out << "  end" << endl;
			    } else {
			    	out << "  // hamming direct output for 57 + 6 = 63 bits" << endl;
			    	out << "next_state[0] = next_state_Hamming[2];" << endl;
			    	out << "next_state[1] = next_state_Hamming[4];" << endl;
			    	out << "next_state[2] = next_state_Hamming[5];" << endl;
			    	out << "next_state[3] = next_state_Hamming[6];" << endl;
			    	out << "next_state[4] = next_state_Hamming[8];" << endl;
			    	out << "next_state[5] = next_state_Hamming[9];" << endl;
			    	out << "next_state[6] = next_state_Hamming[10];" << endl;
			    	out << "next_state[7] = next_state_Hamming[11];" << endl;
			    	out << "next_state[8] = next_state_Hamming[12];" << endl;
			    	out << "next_state[9] = next_state_Hamming[13];" << endl;
			    	out << "next_state[10] = next_state_Hamming[14];" << endl;
			    	out << "next_state[11] = next_state_Hamming[16];" << endl;
			    	out << "next_state[12] = next_state_Hamming[17];" << endl;
			    	out << "next_state[13] = next_state_Hamming[18];" << endl;
			    	out << "next_state[14] = next_state_Hamming[19];" << endl;
			    	out << "next_state[15] = next_state_Hamming[20];" << endl;
			    	out << "next_state[16] = next_state_Hamming[21];" << endl;
			    	out << "next_state[17] = next_state_Hamming[22];" << endl;
			    	out << "next_state[18] = next_state_Hamming[23];" << endl;
			    	out << "next_state[19] = next_state_Hamming[24];" << endl;
			    	out << "next_state[20] = next_state_Hamming[25];" << endl;
			    	out << "next_state[21] = next_state_Hamming[26];" << endl;
			    	out << "next_state[22] = next_state_Hamming[27];" << endl;
			    	out << "next_state[23] = next_state_Hamming[28];" << endl;
			    	out << "next_state[24] = next_state_Hamming[29];" << endl;
			    	out << "next_state[25] = next_state_Hamming[30];" << endl;
			    	out << "next_state[26] = next_state_Hamming[32];" << endl;
			    	out << "next_state[27] = next_state_Hamming[33];" << endl;
			    	out << "next_state[28] = next_state_Hamming[34];" << endl;
			    	out << "next_state[29] = next_state_Hamming[35];" << endl;
			    	out << "next_state[30] = next_state_Hamming[36];" << endl;
			    	out << "next_state[31] = next_state_Hamming[37];" << endl;
			    	out << "next_state[32] = next_state_Hamming[38];" << endl;
			    	out << "next_state[33] = next_state_Hamming[39];" << endl;
			    	out << "next_state[34] = next_state_Hamming[40];" << endl;
			    	out << "next_state[35] = next_state_Hamming[41];" << endl;
			    	out << "next_state[36] = next_state_Hamming[42];" << endl;
			    	out << "next_state[37] = next_state_Hamming[43];" << endl;
			    	out << "next_state[38] = next_state_Hamming[44];" << endl;
			    	out << "next_state[39] = next_state_Hamming[45];" << endl;
			    	out << "next_state[40] = next_state_Hamming[46];" << endl;
			    	out << "next_state[41] = next_state_Hamming[47];" << endl;
			    	out << "next_state[42] = next_state_Hamming[48];" << endl;
			    	out << "next_state[43] = next_state_Hamming[49];" << endl;
			    	out << "next_state[44] = next_state_Hamming[50];" << endl;
			    	out << "next_state[45] = next_state_Hamming[51];" << endl;
			    	out << "next_state[46] = next_state_Hamming[52];" << endl;
			    	out << "next_state[47] = next_state_Hamming[53];" << endl;
			    	out << "next_state[48] = next_state_Hamming[54];" << endl;
			    	out << "next_state[49] = next_state_Hamming[55];" << endl;
			    	out << "next_state[50] = next_state_Hamming[56];" << endl;
			    	out << "next_state[51] = next_state_Hamming[57];" << endl;
			    	out << "next_state[52] = next_state_Hamming[58];" << endl;
			    	out << "next_state[53] = next_state_Hamming[59];" << endl;
			    	out << "next_state[54] = next_state_Hamming[60];" << endl;
			    	out << "next_state[55] = next_state_Hamming[61];" << endl;
			    	out << "next_state[56] = next_state_Hamming[62];" << endl;
			    }
				break;
			default:
				cout << "Number of Hamming-Bits not supported";
				return;
				break;

    	}

    	out << "end" << endl << endl;
    }


    // Write always-block => assigns next_state to state
    //------------------------------

    // Ifdef ASYNC_RES like always definition
    out << "`ifdef ASYNC_RES" << endl;
    out << " always @(posedge " << fsm->getClockName().c_str()
            << " or negedge " << fsm->getResetName().c_str() << " ) begin"
            << endl;
    out << "`else" << endl;
    out << " always @(posedge " << fsm->getClockName().c_str()
            << ") begin" << endl;
    out << "`endif" << endl;

    out << "    if (!" << this->cleanString(fsm->getResetName())
            << ")" << endl;
    out << "    begin" << endl;

    //-- Reset: Current State
    out << "        current_state <= ";
    out << this->cleanString(fsm->getStatebyID(fsm->getResetState())->getName())<< ";" << endl;


    //-- Reset: Current state delayed
    if (this->getParameter("forward.delayed").toBool()) {

        out << "        forward_state_delayed <= ";
        out << this->cleanString(fsm->getStatebyID(fsm->getResetState())->getName()) << ";" << endl;

    }

    out << "    end" << endl;
    out << "    else begin" << endl;

    if (this->getParameter("forward.delayed").toBool()) {
        //-- Output Assign Delayed
        out << "        current_state         <= forward_state_delayed;"
                << endl;
        out << "        forward_state_delayed <= next_state;" << endl;

    } else {
        //-- Output Assign: normal
        out << "        current_state <= next_state;" << endl;
    }

    out << "    end" << endl;
    out << "end" << endl;

    // Verification Extensions, can be used with Cadence formal verifier
    //-----------------------------
    if (Statecov || Transcov) {

        //-- Start of Coverage
        out << endl << "`ifdef CAG_COVERAGE" << endl;
        out << "// synopsys translate_off" << endl << endl;

        //-- Write State coverage
        //-----------------------
        if (this->Statecov) {

            //-- Commentar
            out << "\t// State coverage" << endl << "\t//--------"
                    << endl << endl;
            out << "\t//-- Coverage group declaration" << endl;

            //-- Start of coverage group
            out << "\tcovergroup cg_states @(posedge clk);" << endl;
            out << "\t\tstates : coverpoint current_state {" << endl;

            //-- States
            int statescount = 0;
            FOREACH_STATE(fsm)
                out << "\t\t\tbins "
                        << this->cleanString(state->getName()) << " = {"
                        << this->cleanString(state->getName()) << "};"
                        << endl;
            END_FOREACH_STATE

            //-- End of coverage group
            out << "\t\t}" << endl;
            out << "\tendgroup : cg_states" << endl << endl;

            out << "\t//-- Coverage group instanciation" << endl;
            out << "\tcg_states state_cov_I;" << endl;

            out << "\tinitial begin" << endl;

            out << "\t\tstate_cov_I = new();" << endl;
            out << "\t\tstate_cov_I.set_inst_name(\"state_cov_I\");"
                    << endl;

            out << "\tend" << endl << endl;

        }

        //-- Write transitions coverage
        //-----------------------
        if (this->Transcov) {

            //-- Commentar
            out << "\t// Transitions coverage" << endl << "\t//--------"
                    << endl << endl;

            //---- Go through all possible transitions for every condition
            //-------------------------
            int transitionCount = 0;
            map<string, int> transitionsNames;// Map to solve multiple identical transition names
            FOREACH_TRANSITIONS(fsm)

                //-- Get target state and source state
                QString sstate = this->cleanString(transition->getStartState()->getName());
                QString tstate = this->cleanString(transition->getEndState()->getName());

                // Prepare Transition Name
                //-------------------------
                stringstream transName;
                transName << "tc_"
                        << this->cleanString(transition->getName()).toStdString();

                //-- Unnamed = get a dummy name
                if (transition->getName().size() == 0) {
                    transName << this->cleanString(sstate.toStdString()).toStdString() << "_to_"
                            << this->cleanString(tstate.toStdString()).toStdString();
                }

                //-- Check buildup name doesn_t already exists, if yes, then increment the counter

                // Increment value in map
                if (transitionsNames.count(transName.str()) == 0)
                    transitionsNames.insert(
                            pair<string, int>(transName.str(), 0));
                transitionsNames[transName.str()]++;

                // Suffix count value
                if (transitionsNames[transName.str()] > 1) {
                    transName << transitionsNames[transName.str()];
                }

                //-- Default transition (not condition on input to be checked)
                //------------------------------
                if (transition->isDefault()) {

                    out << "\t"
                        << transName.str().c_str() << "_default"
                        << ": cover property( @(posedge clk) disable iff (!res_n)"
                        << " (current_state == " << sstate << ")"
                        << "|=> (current_state == " << tstate
                        << ") );" << endl << endl;

                }
                //-- Otherwise one cover property per condition
                //------------------------------
                else {
                    int cCount = 0 ;
                    FOREACH_TRANSITION_CONDITIONS(transition)

                        //-- Condition Input
                        string conditionInput = condition->getInput();

                        // Replace '-' with 'x'
                        conditionInput = QString::fromStdString(conditionInput).replace('-',"x").toStdString();

                        //-- Input vector
                        stringstream inputVector;
                        inputVector << "inputvector ==? "
                                    << fsm->getNumberOfInputs() << "'b"
                                    << conditionInput;

                        //-- Property Name
                        stringstream propertyName;
                        propertyName << transName.str();

                        //-- Add Condition index and condition name (if one)
                        if (transition->getConditions().size()>1)
                            propertyName << "_c" << cCount;
                        if (condition->getName().size()>0)
                            propertyName << "_" << this->cleanString(condition->getName()).toStdString();

                        out << "\t"
                            << propertyName.str().c_str()
                            << ": cover property( @(posedge clk) disable iff (!res_n)"
                            << "(" << inputVector.str().c_str() << ") &&"
                            << "(current_state == " << sstate << ")"
                            << "|=> (current_state == " << tstate << ")"
                            << ");" << endl << endl;

                        cCount++;

                    END_FOREACH_TRANSITION_CONDITIONS


                }

             END_FOREACH_TRANSITIONS

        } // End of Transcoverage

        //-- End of coverage
        out << "// synopsys translate_on" << endl;
        out << "`endif" << endl << endl;
    }

    out << endl << "endmodule" << endl;


    // Save file path
   // fsm->setLastGeneratedVerilogFile(out);

}



QString VerilogSecuredGenerator::createInstance(Fsm * fsm) {

    stringstream ss;

    //-- Wires declarations
    //----------------------------

    //-- Inputs
    for (int i = 0; i < fsm->getNumberOfInputs(); i++) {
        ss << "reg fsm_" << fsm->getInputName(i) << ";" << endl;
    }
    ss << endl;

    //-- Outputs
    for (int i = 0; i < fsm->getNumberOfOutputs(); i++) {
        ss << "wire fsm_" << fsm->getOutputName(i) << ";" << endl;
    }

    //-- Forward declaration
    /*if (this->getForwardState()) {
        ss << "wire [" << this->numberofoutputs - 1 << ":0] fsm_forward_state;"
                << endl;
    }
    if (this->getForwardStateAsync()) {
        ss << "wire [" << this->numberofoutputs - 1
                << ":0] fsm_forward_state_async;" << endl;
    }
    if (this->getForwardStateDelayed()) {
        ss << "wire [" << this->numberofoutputs - 1
                << ":0] fsm_forward_state_delayed;" << endl;
    }*/
    ss << endl;

    // Instance
    //------------------------------

    //-- Instance declaration
    ss << fsm->getFsmName() << " " << fsm->getFsmName() << "_I (" << endl << endl;

    //-- Clocks and Resets
    ss << "    ." << fsm->getClockName().c_str() << "(), "
                << endl;
    ss << "    ." << fsm->getResetName().c_str() << "(), "
                   << endl;

    //-- Inputs
    ss << "\t//-- Inputs" << endl;
    for (int i = 0; i < fsm->getNumberOfInputs(); i++) {
        ss << "\t." << fsm->getInputName(i) << "(" << "fsm_"
                << fsm->getInputName(i) << "), " << endl;
    }
    ss << endl;

    //-- Outputs
    ss << "\t//-- Outputs" << endl;
    for (int i = 0; i < fsm->getNumberOfOutputs() - 1; i++) {
        ss << "\t." << fsm->getOutputName(i) << "(" << "fsm_"
                << fsm->getOutputName(i) << "), " << endl;
    }
    ss
            << "\t."
            << fsm->getOutputName(fsm->getNumberOfOutputs() - 1)
            << "("
            << "fsm_"
            << fsm->getOutputName(fsm->getNumberOfOutputs() - 1)
            << ")" << endl ;
            /*<< ((this->getForwardStateAsync() || this->getForwardStateDelayed()
                    || this->getForwardState()) ? "," : "") << endl;*/

    //-- Forward declaration
    /*if (this->getForwardStateAsync()) {
        ss << endl << "\t//-- Forward Async" << endl;
        ss << "\t.forward_state_async(fsm_forward_state_async)";
        if (this->getForwardStateDelayed() || this->getForwardState())
            ss << ",";
        ss << endl;

    }
    if (this->getForwardStateDelayed()) {
        ss << endl << "\t//-- Forward delayed" << endl;
        ss << "\t.forward_state_delayed(fsm_forward_state_delayed)";
        if (this->getForwardState())
            ss << ",";
        ss << endl;
    }
    if (this->getForwardState()) {
        ss << endl << "\t//-- Forwarded actual state" << endl;
        ss << "\t.forward_state(fsm_forward_state)" << endl;
    }*/

    //-- End instance
    ss << ");" << endl;

    return QString::fromStdString(ss.str());
}

QString VerilogSecuredGenerator::cleanString(string input) {

    //-- Use QString
    QString inputQString(input.c_str());

    // Replace spaces with _
    //------------------------
    inputQString = inputQString.replace(QRegExp("\\s+"), QChar('_'));

    //-- replaces then non \\W characters by nothing
    //inputQString = inputQString.replace(QRegExp("\\W"),"");

    //-- Only Allow letters and numbers
    inputQString = inputQString.replace(QRegExp("[^A-Za-z0-9_]*"), "");

    return inputQString;

}

