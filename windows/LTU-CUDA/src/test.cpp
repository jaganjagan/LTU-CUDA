#include <iostream>
#include <memory>
#include <cassert>
#include <thrust/device_ptr.h>
#include <thrust/fill.h>
#include "ltucuda/pgm/pgm.cuh"
#include "ltucuda/ltucuda.cuh"
#include "ltucuda/kernels/transpose.cuh"
#include "ltucuda/morphology/morphology.cuh"

void setDevice();
int resetDevice();
void showMemUsage();

/*
 * General method for tests that erode an input image with a morphMask and write the result to filenameOut if != NULL
 */ 
void  testErosion(cudaPaddedImage input, cudaImage output, rect2d roi, morphMask mask, const char* filenameOut, const char* textOnError) {
	performErosion(getData(input), getPitch(input), getData(output), getPitch(output), roi, mask, input.border);

	if (filenameOut != NULL) {
		float *host_out = copyImageToHost(output);
		printf("output width: %d , height: %d\n", output.width, output.height);
		savePGM(filenameOut, host_out, output.width, output.height);
		freeHost(host_out, PINNED);
	}
	exitOnError(textOnError);
}

int main( int argc, const char* argv[] )
{
	setDevice();
	float maxFloat = std::numeric_limits<float>::max();
	// Prepad border bigger than we need for simplicity and profiling (256px border is silly big; we can have an SE length of 512).	
	rect2d border = { 256,256 }; 
	cudaPaddedImage paddedImage = loadPaddedImageToDevice("../images/test.pgm", border, 255.0f);
	rect2d imageSize = { paddedImage.image.width - 2 * border.width, paddedImage.image.height- 2 * border.height };
	cudaImage output = createImage(imageSize.width, imageSize.height, 255.0f);

	float *host_out = copyImageToHost(paddedImage.image);
	savePGM("fillTest.pgm", host_out, paddedImage.image.width, paddedImage.image.height);
	freeHost(host_out, PINNED);
	//return 0;
	/*
		Diagonal erosion 3x3
	*/
	unsigned char diag[] = {1,0,0,0,1,0,0,0,1};
	morphMask diag3x3mask = createTBTMask(diag);
	testErosion(paddedImage, output, imageSize, diag3x3mask, "diag3x3.pgm", "Diagonal VHGW Test");

	/* 
		VHGW erosion: Horizontal, vertical, diagonal
	*/
	morphMask hozMask  = createVHGWMask(43,  HORIZONTAL);
	morphMask vertMask = createVHGWMask(43, VERTICAL);
	morphMask diagMask = createVHGWMask(51, DIAGONAL_BACKSLASH);
	//testErosion(paddedImage, output, imageSize, hozMask, "horizontalVHGW.pgm", "VHGW Horizontal kernel"); // replaced with TVT below
	testErosion(paddedImage, output, imageSize, diagMask, "diagonalVHGW.pgm", "VHGW Diagonal kernel");
	testErosion(paddedImage, output, imageSize, vertMask, "verticalVHGW.pgm", "VHGW Vertical kernel");	
	
	// Horizontal Erosion Test Mark 2: Transpose + Vertical+ Transpose
	cudaImage flippedImage = createTransposedImage(paddedImage.image);
	cudaFree(getData(paddedImage));

	rect2d flippedSize = {imageSize.height, imageSize.width};
	cudaImage flippedOut;
	flippedOut.width = flippedSize.width;
	flippedOut.height = flippedSize.height;
	allocImageOnDevice(flippedOut);

	rect2d flippedBorder = {border.height, border.width};
	performErosion(getData(flippedImage), getPitch(flippedImage), getData(flippedOut), getPitch(flippedOut), flippedSize, vertMask, flippedBorder);
	exitOnError("VHGW Horizontal Transpose Test");
	
	cudaFree(getData(flippedImage));
	flippedImage = createTransposedImage(flippedOut);
	host_out = copyImageToHost(flippedImage);
	savePGM("horizontalTransposeVHGW.pgm", host_out, flippedImage.width, flippedImage.height);
	freeHost(host_out, PINNED);

	/*
		31 erosions with 3x3
	*/
	/*unsigned char diag3[] = {0,0,1,0,1,0,1,0,0};
	morphMask diagonalTbt = createTBTMask(diag3);

	for(int i = 0; i < 7; i++) {
		performErosion(getData(paddedImage), getPitch(paddedImage), getBorderOffsetImagePtr(paddedOut), getPitch(paddedOut), imageSize, diagonalTbt, border);
		performErosion(getData(paddedOut), getPitch(paddedOut), getBorderOffsetImagePtr(paddedImage), getPitch(paddedImage), imageSize, diagonalTbt, border);
		if (i == 6)
			performErosion(getData(paddedImage), getPitch(paddedImage), getData(image), getPitch(image), imageSize, diagonalTbt, border);
	}
	host_out = copyImageToHost(image);
	savePGM("diagTest2.pgm", host_out, image.width, image.height);
	freeHost(host_out, PINNED);*/


	cudaFree(getData(flippedImage));
	cudaFree(getData(flippedOut));
    cudaFree(hozMask.data);
	cudaFree(vertMask.data);
	cudaFree(diagMask.data);
	cudaFree(output.data);

	printf("After:");
	//showMemUsage();
	printf("-------------------\nAll tests Done.");
	return resetDevice();
}

void setDevice() {
	cudaError_t cuda_status = cudaSetDevice(0);
	if ( cudaSuccess != cuda_status ){
        printf("Error: Couldn't select device: %s\n", cudaGetErrorString(cuda_status) );
        exit(1);
    }
}

int resetDevice() {
	// Reset device; needed for profiler
    cudaError_t cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
        return 1;
    }
    return 0;
}

void showMemUsage() {
	// show memory usage of GPU
    /*size_t free_byte ;
    size_t total_byte ;
    cudaError_t cuda_status = cudaMemGetInfo( &free_byte, &total_byte ) ;
    if ( cudaSuccess != cuda_status ){
        printf("Error: cudaMemGetInfo fails, %s \n", cudaGetErrorString(cuda_status) );
        exit(1);
    }

    double free_db = (double)free_byte ;
    double total_db = (double)total_byte ;
    double used_db = total_db - free_db ;
    printf("GPU memory usage: used = %f, free = %f MB, total = %f MB\n",
        used_db/1024.0/1024.0, free_db/1024.0/1024.0, total_db/1024.0/1024.0);*/
}