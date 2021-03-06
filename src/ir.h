/**
 * @file   ir.h
 * @date   02/2018
 * @author Imran Ashraf
 * @brief  common IR implementation
 */

#ifndef IR_H
#define IR_H

#include "gate.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <list>

namespace ql
{
    namespace ir
    {
        typedef std::list<ql::gate *>section_t;
        class bundle_t
        {
        public:
            size_t start_cycle;                         // start cycle for all gates in parallel_sections
            size_t duration_in_cycles;                  // the maximum gate duration in parallel_sections
            std::list<section_t> parallel_sections;
        };

        typedef std::list<bundle_t>bundles_t;           // note that subsequent bundles can overlap in time

        inline std::string qasm(bundles_t & bundles)
        {
            std::stringstream ssqasm;
            size_t curr_cycle=1;

            ssqasm << '\n';
            for (bundle_t & abundle : bundles)
            {
                auto st_cycle = abundle.start_cycle;
                auto delta = st_cycle - curr_cycle;
                // DOUT("Printing bundle with st_cycle: " << st_cycle);
                if(delta>1)
                    ssqasm << "    wait " << delta-1 << '\n';
                // else
                //   ssqasm << '\n';

                auto ngates = 0;
                for( auto sec_it = abundle.parallel_sections.begin(); sec_it != abundle.parallel_sections.end(); ++sec_it )
                {
                    ngates += sec_it->size();
                }
                ssqasm << "    ";
                if (ngates > 1) ssqasm << "{ ";
                auto isfirst = 1;
                for( auto sec_it = abundle.parallel_sections.begin(); sec_it != abundle.parallel_sections.end(); ++sec_it )
                {
                    for ( auto gp : (*sec_it))
                    {
                        if (isfirst == 0)
                            ssqasm << " | ";
                        ssqasm << gp->qasm();
                        isfirst = 0;
                    }
                }
                if (ngates > 1) ssqasm << " }";
                curr_cycle+=delta;
                ssqasm << "\n";
            }

            if( !bundles.empty() )
            {
                auto & last_bundle = bundles.back();
                int lsduration = last_bundle.duration_in_cycles;
                if( lsduration > 1 )
                    ssqasm << "    wait " << lsduration -1 << '\n';
            }

            return ssqasm.str();
        }

        inline void write_qasm(bundles_t & bundles)
        {
            std::ofstream fout;
            std::string fname( ql::options::get("output_dir") + "/ir.qasm" );
            fout.open( fname, std::ios::binary);
            if ( fout.fail() )
            {
                EOUT("Error opening file " << fname << std::endl
                         << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
                return;
            }

            fout << qasm(bundles);
            fout.close();
        }


        // return bundles for the given circuit;
        // assumes gatep->cycle attribute reflects the cycle assignment;
        // assumes circuit being a vector of gate pointers is ordered by this cycle value;
        // create bundles in a single scan over the circuit, using currBundle and currCycle as state
        inline bundles_t bundler(ql::circuit& circ, size_t cycle_time)
        {
            bundles_t bundles;          // result bundles
        
            bundle_t    currBundle;     // current bundle at currCycle that is being filled
            size_t      currCycle = 0;  // cycle at which bundle is to be scheduled
    
            currBundle.start_cycle = currCycle; // starts off as empty bundle starting at currCycle
            currBundle.duration_in_cycles = 0;
    
            DOUT("bundler ...");
    
            for (auto & gp: circ)
            {
                DOUT(". adding gate(@" << gp->cycle << ")  " << gp->qasm());
                if ( gp->type() == ql::gate_type_t::__wait_gate__ ||
                     gp->type() == ql::gate_type_t::__dummy_gate__
                   )
                {
                    DOUT("... ignoring: " << gp->qasm());
                    continue;
                }
                size_t newCycle = gp->cycle;        // taking cycle values from circuit, so excludes SOURCE and SINK!
                if (newCycle < currCycle)
                {
                    EOUT("Error: circuit not ordered by cycle value");
                    throw ql::exception("[x] Error: circuit not ordered by cycle value",false);
                }
                if (newCycle > currCycle)
                {
                    if (!currBundle.parallel_sections.empty())
                    {
                        // finish currBundle at currCycle
                        DOUT(".. bundle at cycle " << currCycle << " duration in cycles: " << currBundle.duration_in_cycles);
                        for (auto &s : currBundle.parallel_sections)
                        {
                            for (auto &sgp: s)
                            {
                                DOUT("... with gate(@" << sgp->cycle << ")  " << sgp->qasm());
                            }
                        }
                        bundles.push_back(currBundle);
                        DOUT(".. ready with bundle at cycle " << currCycle);
                        currBundle.parallel_sections.clear();
                    }
    
                    // new empty currBundle at newCycle
                    currCycle = newCycle;
                    DOUT(".. bundling at cycle: " << currCycle);
                    currBundle.start_cycle = currCycle;
                    currBundle.duration_in_cycles = 0;
                }
    
                // add gp to currBundle
                section_t asec;
                asec.push_back(gp);
                currBundle.parallel_sections.push_back(asec);
                DOUT("... gate: " << gp->qasm() << " in private parallel section");
                currBundle.duration_in_cycles = std::max(currBundle.duration_in_cycles, (gp->duration+cycle_time-1)/cycle_time); 
            }
            if (!currBundle.parallel_sections.empty())
            {
                // finish currBundle (which is last bundle) at currCycle
                DOUT("... bundle at cycle " << currCycle << " duration in cycles: " << currBundle.duration_in_cycles);
                for (auto &s : currBundle.parallel_sections)
                {
                    for (auto &sgp: s)
                    {
                        DOUT("... with gate(@" << sgp->cycle << ")  " << sgp->qasm());
                    }
                }
                bundles.push_back(currBundle);
                DOUT(".. ready with bundle at cycle " << currCycle);
            }
    
            // currCycle == cycle of last gate of circuit scheduled
            // duration_in_cycles later the system starts idling
            // depth is the difference between the cycle in which it starts idling and the cycle it started execution
            if (bundles.empty())
            {
                DOUT("Depth: " << 0);
            }
            else
            {
                DOUT("Depth: " << currCycle + currBundle.duration_in_cycles - bundles.front().start_cycle);
            }
            DOUT("bundler [DONE]");
            return bundles;
        }

        inline void DebugBundles(std::string at, bundles_t& bundles)
        {
            DOUT("DebugBundles at: " << at << " showing " << bundles.size() << " bundles");
            for (bundle_t & abundle : bundles)
            {
                DOUT("... bundle with nsections: " << abundle.parallel_sections.size());
                for( auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt )
                {
                    DOUT("... section with ngates: " << secIt->size());
                    for ( auto gp : (*secIt))
                    {
                        // auto n = get_cc_light_instruction_name(gp->name, platform);
                        DOUT("... ... gate: " << gp->qasm() << " name: " << gp->name << " cc_light_iname: " << "?");
                    }
                }
            }
        }

    } // namespace ir
} //namespace ql

#endif
