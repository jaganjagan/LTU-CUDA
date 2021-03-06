#include "ltucuda/matlabBridge/matlabBridge.h"
#include "ltucuda/pgm/pgm.cuh"

void mexFunction(int nlhs, mxArray *plhs[],
		int nrhs, const mxArray *prhs[])
{
	/*size_t free = 0, total = 0;
    cudaError_t result = cudaMemGetInfo(&free, &total);

    mexPrintf("free memory in bytes %u (%u MB), total memory in bytes %u (%u MB). ", free, free/1024/1024, total, total/1024/1024);

    if( total > 0 )
        mexPrintf("%2.2f%% free\n", (100.0*free)/total );
    else
        mexPrintf("\n");
	*/
	cudaImage image = allocImage(); //imageFromMXArray(mx);
	plhs[0] = imageToMXStruct(image);

    //const mxArray *mx = prhs[0];
    //if (mxIsSingle(mx)) {
	/*else if (mxIsUint8(mx)) {
	    cudaImage image = cudaImageFrom8bitMX(const mx);
      	plhs[0] = cudaMatrixToStruct(image);

	} */
}
