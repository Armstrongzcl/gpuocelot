/*! \file PTXOptimzer.cpp
	\date Thursday December 31, 2009
	\author Gregory Diamos <gregory.diamos@gatech.edu>
	\brief The source file for the Ocelot PTX optimizer
*/

#ifndef PTX_OPTIMIZER_CPP_INCLUDED
#define PTX_OPTIMIZER_CPP_INCLUDED

// Ocelot Includes
#include <ocelot/tools/PTXOptimizer.h>
#include <ocelot/transforms/interface/PassManager.h>
#include <ocelot/transforms/interface/LinearScanRegisterAllocationPass.h>
#include <ocelot/transforms/interface/RemoveBarrierPass.h>
#include <ocelot/transforms/interface/StructuralTransform.h>
#include <ocelot/transforms/interface/ConvertPredicationToSelectPass.h>
#include <ocelot/transforms/interface/SubkernelFormationPass.h>
#include <ocelot/transforms/interface/MIMDThreadSchedulingPass.h>
#include <ocelot/transforms/interface/DeadCodeEliminationPass.h>
#include <ocelot/transforms/interface/SplitBasicBlockPass.h>
#include <ocelot/transforms/interface/SyncEliminationPass.h>
#include <ocelot/transforms/interface/HoistSpecialValueDefinitionsPass.h>
#include <ocelot/transforms/interface/SimplifyControlFlowGraphPass.h>

#include <ocelot/ir/interface/Module.h>

// Hydrazine Includes
#include <hydrazine/interface/ArgumentParser.h>
#include <hydrazine/interface/Exception.h>
#include <hydrazine/interface/string.h>
#include <hydrazine/interface/debug.h>

// Standard Library Includes
#include <fstream>

// Preprocessor Macros
#ifdef REPORT_BASE
#undef REPORT_BASE
#endif

#define REPORT_BASE 0

namespace tools
{
	PTXOptimizer::PTXOptimizer() : registerAllocationType( 
		InvalidRegisterAllocationType ), passes( 0 )
	{
	
	}

	void PTXOptimizer::optimize()
	{		
		report("Running PTX to PTX Optimizer.");
		
		report(" Loading module '" << input << "'");
		ir::Module module( input );

		transforms::PassManager manager( &module );

		if( registerAllocationType == LinearScan )
		{
			transforms::Pass* pass =
				new transforms::LinearScanRegisterAllocationPass( 
				registerCount );
			manager.addPass( *pass );
		}

		if( passes & SubkernelFormation )
		{
			transforms::Pass* pass = new transforms::SubkernelFormationPass(
				subkernelSize );
			manager.addPass( *pass );
		}
		
		if( passes & RemoveBarriers )
		{
			transforms::Pass* pass = new transforms::RemoveBarrierPass;
			manager.addPass( *pass );
		}

		if( passes & StructuralTransform )
		{
			transforms::Pass* pass = new transforms::StructuralTransform;
			manager.addPass( *pass );
		}

		if( passes & ReverseIfConversion )
		{
			transforms::Pass* pass =
				new transforms::ConvertPredicationToSelectPass;
			manager.addPass( *pass );
		}

		if( passes & MIMDThreadScheduling )
		{
			transforms::Pass* pass = new transforms::MIMDThreadSchedulingPass;
			manager.addPass( *pass );
		}

		if( passes & DeadCodeElimination )
		{
			transforms::Pass* pass = new transforms::DeadCodeEliminationPass;
			manager.addPass( *pass );
		}
		
		if( passes & SplitBasicBlocks )
		{
			transforms::Pass* pass = new transforms::SplitBasicBlockPass(
				basicBlockSize );
			manager.addPass( *pass );
		}
		
		if( passes & SyncElimination )
		{
			transforms::Pass* pass = new transforms::SyncEliminationPass;
			manager.addPass( *pass );
		}
		
		if( passes & HoistSpecialDefinitions )
		{
			transforms::Pass* pass =
				new transforms::HoistSpecialValueDefinitionsPass;
			manager.addPass( *pass );
		}
		
		if( passes & SimplifyCFG )
		{
			transforms::Pass* pass =
				new transforms::SimplifyControlFlowGraphPass;
			manager.addPass( *pass );
		}
		
		if( input.empty() )
		{
			std::cout << "No input file name given.  Bailing out." << std::endl;
			return;
		}

		manager.runOnModule();
		manager.destroyPasses();
		
		std::ofstream out( output.c_str() );
		
		if( !out.is_open() )
		{
			throw hydrazine::Exception( "Could not open output file " 
				+ output + " for writing." );
		}
		
		module.writeIR( out );

		if(!cfg) return;
		
		for( ir::Module::KernelMap::const_iterator 
			kernel = module.kernels().begin(); 
			kernel != module.kernels().end(); ++kernel )
		{
			report(" Writing CFG for kernel '" << kernel->first << "'");
			std::ofstream out( std::string( 
				kernel->first + "_cfg.dot" ).c_str() );
		
			if( !out.is_open() )
			{
				throw hydrazine::Exception( "Could not open output file " 
					+ output + " for writing." );
			}
		
			module.getKernel( kernel->first )->cfg()->write( out );
		}
	}
}

static int parsePassTypes( const std::string& passList )
{
	int types = tools::PTXOptimizer::InvalidPassType;
	
	report("Checking for pass types.");
	hydrazine::StringVector passes = hydrazine::split( passList, "," );
	for( hydrazine::StringVector::iterator pass = passes.begin(); 
		pass != passes.end(); ++pass )
	{
		*pass = hydrazine::strip( *pass, " " );
		report( " Checking option '" << *pass << "'" );
		if( *pass == "remove-barriers" )
		{
			report( "  Matched remove-barriers." );
			types |= tools::PTXOptimizer::RemoveBarriers;
		}
		else if( *pass == "reverse-if-conversion" )
		{
			report( "  Matched reverse-if-conversion." );
			types |= tools::PTXOptimizer::ReverseIfConversion;
		}
		else if( *pass == "structural-transform" )
		{
			report( "  Matched structural-transform." );
			types |= tools::PTXOptimizer::StructuralTransform;
		}
		else if( *pass == "subkernel-formation" )
		{
			report( "  Matched subkernel-formation." );
			types |= tools::PTXOptimizer::SubkernelFormation;
		}
		else if( *pass == "mimd-threading" )
		{
			report( "  Matched mimd-threading." );
			types |= tools::PTXOptimizer::MIMDThreadScheduling;
		}
		else if( *pass == "dead-code-elimination" )
		{
			report( "  Matched dead-code-elimination." );
			types |= tools::PTXOptimizer::DeadCodeElimination;
		}
		else if( *pass == "split-blocks" )
		{
			report( "  Matched split-blocks." );
			types |= tools::PTXOptimizer::SplitBasicBlocks;
		}
		else if( *pass == "sync-elimination" )
		{
			report( "  Matched sync-elimination." );
			types |= tools::PTXOptimizer::SyncElimination;
		}
		else if( *pass == "hoist-special-definitions" )
		{
			report( "  Matched hoist-special-definitions." );
			types |= tools::PTXOptimizer::HoistSpecialDefinitions;
		}
		else if( *pass == "simplify-cfg" )
		{
			report( "  Matched simplify-cfg." );
			types |= tools::PTXOptimizer::SimplifyCFG;
		}
		else if( !pass->empty() )
		{
			std::cout << "==Ocelot== Warning: Unknown pass name - '" << *pass 
				<< "'\n";
		}
	}
	return types;
}

int main( int argc, char** argv )
{
	hydrazine::ArgumentParser parser( argc, argv );
	parser.description( "The Ocelot PTX to PTX optimizer." );
	tools::PTXOptimizer optimizer;
	std::string allocator;
	std::string passes;
	
	parser.parse( "-i", "--input", optimizer.input, "",
		"The ptx file to be optimized." );
	parser.parse( "-o", "--output", optimizer.output, 
		"_optimized_" + optimizer.input, "The resulting optimized file." );
	parser.parse( "-a", "--allocator", allocator, "none",
		"The type of register allocator to use (linearscan)." );
	parser.parse( "-r", "--max-registers", optimizer.registerCount, 32,
		"The number of registers available for allocation." );
	parser.parse( "-s", "--subkernel-size", optimizer.subkernelSize, 70,
		"The target size for subkernel formation." );
	parser.parse( "-b", "--block-size", optimizer.basicBlockSize, 50,
		"The target size for block splitting." );
	parser.parse( "-p", "--passes", passes, "", 
		"A list of optimization passes (remove-barriers, " 
		"reverse-if-conversion, subkernel-formation, structural-transform, "
		"mimd-threading, dead-code-elimination, split-blocks, "
		"sync-elimination, hoist-special-definitions, simplify-cfg)" );
	parser.parse( "-c", "--cfg", optimizer.cfg, false, 
		"Dump out the CFG's of all generated kernels." );
	parser.parse();

	if( allocator == "linearscan" )
	{
		optimizer.registerAllocationType = tools::PTXOptimizer::LinearScan;
	}
	
	optimizer.passes = parsePassTypes( passes );
	
	optimizer.optimize();

	return 0;
}

#endif

