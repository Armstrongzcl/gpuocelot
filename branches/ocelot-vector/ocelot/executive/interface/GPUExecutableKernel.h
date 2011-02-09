/*! \file GPUExecutableKernel.h
	\author Andrew Kerr <arkerr@gatech.edu>
	\date Jan 19, 2009
	\brief implements the GPU kernel callable by the executive
*/

#ifndef EXECUTIVE_GPUKERNEL_H_INCLUDED
#define EXECUTIVE_GPUKERNEL_H_INCLUDED
#include <ocelot/cuda/interface/CudaDriver.h>

#include <ocelot/ir/interface/PTXKernel.h>
#include <ocelot/ir/interface/ExecutableKernel.h>

namespace executive {

	class GPUExecutableKernel: public ir::ExecutableKernel {
	public:
		GPUExecutableKernel( ir::Kernel& kernel, const CUfunction& function, 
			const executive::Executive* c = 0 );
		GPUExecutableKernel();
		~GPUExecutableKernel();
	
		/*!
			Launch a kernel on a 2D grid
		*/
		void launchGrid(int width, int height);

		/*!
			Sets the shape of a kernel
		*/
		void setKernelShape(int x, int y, int z);

		/*!
			Sets the device used to execute the kernel
		*/
		void setDevice(const Device* device, unsigned int limit);

		/*!
			sets the size of shared memory in bytes
		*/
		void setExternSharedMemorySize(unsigned int bytes);

		void updateParameterMemory();
		
		/*! \brief Indicate that other memory has been updated */
		void updateMemory();
		
		/*! \brief Get a vector of all textures references by the kernel */
		ir::StringSet textureReferences() const;

		/*!  \brief get a set of all identifiers used as addresses by the kernel */
		ir::StringSet addressReferences() const;

		void updateGlobalMemory();

		void updateConstantMemory();

		/*!	adds a trace generator to the EmulatedKernel */
		void addTraceGenerator(trace::TraceGenerator *generator);
		/*!	removes a trace generator from an EmulatedKernel */
		void removeTraceGenerator(trace::TraceGenerator *generator);
		
	protected:
		/*!
			Configures the parameter block for the CUDA driver API
		*/
		void configureParameters();
		
		/*!
			Construct 
		*/
		bool initialize();
	
	protected:
		
		/*!
			PTX Kernel
		*/
		ir::PTXKernel *ptxKernel;
		
		/*!
			CUDA function refering to this kernel
		*/
		CUfunction cuFunction;
		
		friend class executive::Executive;

	};
	
}

#endif