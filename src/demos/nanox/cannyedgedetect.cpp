#include <math.h>
#include <string.h>

extern "C" {
#include "mwtypes.h"

PMWIMAGEHDR CannyEdgeDetect(PMWIMAGEHDR pimage);
};

/**
 * This software has been released into the public domain.
 * Please read the notes in this source file for additional information.
 * 
 * This class provides a configurable implementation of the Canny edge
 * detection algorithm. This classic algorithm has a number of shortcomings,
 * but remains an effective tool in many scenarios. This class is designed
 * for single threaded use only.
 * 
 * Sample usage:
 * 
 * //create the detector
 * CannyEdgeDetector detector = new CannyEdgeDetector();
 * //adjust its parameters as desired
 * detector.setLowThreshold(0.5f);
 * detector.setHighThreshold(1f);
 * //apply it to an image
 * detector.setSourceImage(frame);
 * detector.process();
 * BufferedImage edges = detector.getEdgesImage();
 * 
 * For a more complete understanding of this edge detector's parameters
 * consult an explanation of the algorithm.
 * 
 * @author Tom Gibara
 *
 */

#define abs(x)		MWABS(x)

	// statics
	const float GAUSSIAN_CUT_OFF = 0.005;
	const float MAGNITUDE_SCALE = 100.0;
	const float MAGNITUDE_LIMIT = 1000.0;
	const int MAGNITUDE_MAX = (int) (MAGNITUDE_SCALE * MAGNITUDE_LIMIT);

class CannyEdgeDetector {

private:

	// fields
	
	int height;
	int width;
	int picsize;
	int* data;
	int* magnitude;
	PMWIMAGEHDR sourceImage;
	PMWIMAGEHDR edgesImage;
	
	float gaussianKernelRadius;
	float lowThreshold;
	float highThreshold;
	int gaussianKernelWidth;
	int contrastNormalized;

	float* xConv;
	float* yConv;
	float* xGradient;
	float* yGradient;
	
public:
	// constructors
	
	/**
	 * Constructs a new detector with default parameters.
	 */
	
	CannyEdgeDetector() {
		lowThreshold = 2.5;
		highThreshold = 7.5;
		gaussianKernelRadius = 2;
		gaussianKernelWidth = 16;
		contrastNormalized = false;
	}

	// accessors
	
	/**
	 * The image that provides the luminance data used by this detector to
	 * generate edges.
	 * 
	 * @return the source image, or null
	 */
	
	PMWIMAGEHDR getSourceImage() {
		return sourceImage;
	}
	
	/**
	 * Specifies the image that will provide the luminance data in which edges
	 * will be detected. A source image must be set before the process method
	 * is called.
	 *  
	 * @param image a source of luminance data
	 */
	
	void setSourceImage(PMWIMAGEHDR image) {
		sourceImage = image;
	}

	/**
	 * Obtains an image containing the edges detected during the last call to
	 * the process method. The buffered image is an opaque image of type
	 * BufferedImage.TYPE_INT_ARGB in which edge pixels are white and all other
	 * pixels are black.
	 * 
	 * @return an image containing the detected edges, or null if the process
	 * method has not yet been called.
	 */
	
	PMWIMAGEHDR getEdgesImage() {
		return edgesImage;
	}
 
	/**
	 * Sets the edges image. Calling this method will not change the operation
	 * of the edge detector in any way. It is intended to provide a means by
	 * which the memory referenced by the detector object may be reduced.
	 * 
	 * @param edgesImage expected (though not required) to be null
	 */
	
	void setEdgesImage(PMWIMAGEHDR a_edgesImage) {
		edgesImage = a_edgesImage;
	}

	/**
	 * The low threshold for hysteresis. The default value is 2.5.
	 * 
	 * @return the low hysteresis threshold
	 */
	
	float getLowThreshold() {
		return lowThreshold;
	}
	
	/**
	 * Sets the low threshold for hysteresis. Suitable values for this parameter
	 * must be determined experimentally for each application. It is nonsensical
	 * (though not prohibited) for this value to exceed the high threshold value.
	 * 
	 * @param threshold a low hysteresis threshold
	 */
	
	void setLowThreshold(float threshold) {
		//if (threshold < 0) throw new IllegalArgumentException();
		lowThreshold = threshold;
	}
 
	/**
	 * The high threshold for hysteresis. The default value is 7.5.
	 * 
	 * @return the high hysteresis threshold
	 */
	
	float getHighThreshold() {
		return highThreshold;
	}
	
	/**
	 * Sets the high threshold for hysteresis. Suitable values for this
	 * parameter must be determined experimentally for each application. It is
	 * nonsensical (though not prohibited) for this value to be less than the
	 * low threshold value.
	 * 
	 * @param threshold a high hysteresis threshold
	 */
	
	void setHighThreshold(float threshold) {
		//if (threshold < 0) throw new IllegalArgumentException();
		highThreshold = threshold;
	}

	/**
	 * The number of pixels across which the Gaussian kernel is applied.
	 * The default value is 16.
	 * 
	 * @return the radius of the convolution operation in pixels
	 */
	
	int getGaussianKernelWidth() {
		return gaussianKernelWidth;
	}
	
	/**
	 * The number of pixels across which the Gaussian kernel is applied.
	 * This implementation will reduce the radius if the contribution of pixel
	 * values is deemed negligable, so this is actually a maximum radius.
	 * 
	 * @param gaussianKernelWidth a radius for the convolution operation in
	 * pixels, at least 2.
	 */
	
	void setGaussianKernelWidth(int a_gaussianKernelWidth) {
		//if (a_gaussianKernelWidth < 2) throw new IllegalArgumentException();
		gaussianKernelWidth = a_gaussianKernelWidth;
	}

	/**
	 * The radius of the Gaussian convolution kernel used to smooth the source
	 * image prior to gradient calculation. The default value is 16.
	 * 
	 * @return the Gaussian kernel radius in pixels
	 */
	
	float getGaussianKernelRadius() {
		return gaussianKernelRadius;
	}
	
	/**
	 * Sets the radius of the Gaussian convolution kernel used to smooth the
	 * source image prior to gradient calculation.
	 * 
	 * @return a Gaussian kernel radius in pixels, must exceed 0.1f.
	 */
	
	void setGaussianKernelRadius(float a_gaussianKernelRadius) {
		//if (a_gaussianKernelRadius < 0.1) throw new IllegalArgumentException();
		gaussianKernelRadius = a_gaussianKernelRadius;
	}
	
	/**
	 * Whether the luminance data extracted from the source image is normalized
	 * by linearizing its histogram prior to edge extraction. The default value
	 * is false.
	 * 
	 * @return whether the contrast is normalized
	 */
	
	int isContrastNormalized() {
		return contrastNormalized;
	}
	
	/**
	 * Sets whether the contrast is normalized
	 * @param contrastNormalized true if the contrast should be normalized,
	 * false otherwise
	 */
	
	void setContrastNormalized(int a_contrastNormalized) {
		contrastNormalized = a_contrastNormalized;
	}
	
	// methods
	
	void process() {
		width = sourceImage->width;
		height = sourceImage->height;
		picsize = width * height;
		initArrays();
		readLuminance();
		readLuminance();
		if (contrastNormalized) normalizeContrast();
		computeGradients(gaussianKernelRadius, gaussianKernelWidth);
		int low = round(lowThreshold * MAGNITUDE_SCALE);
		int high = round( highThreshold * MAGNITUDE_SCALE);
		performHysteresis(low, high);
		thresholdEdges();
		writeEdges(data);
	}
 
private:
	// private utility methods
	
	void initArrays() {
		//if (data == 0) {
			data = new int[picsize];
			magnitude = new int[picsize];

			xConv = new float[picsize];
			yConv = new float[picsize];
			xGradient = new float[picsize];
			yGradient = new float[picsize];
		//}
	}
	
	//NOTE: The elements of the method below (specifically the technique for
	//non-maximal suppression and the technique for gradient computation)
	//are derived from an implementation posted in the following forum (with the
	//clear intent of others using the code):
	//  http://forum.java.sun.com/thread.jspa?threadID=546211&start=45&tstart=0
	//My code effectively mimics the algorithm exhibited above.
	//Since I don't know the providence of the code that was posted it is a
	//possibility (though I think a very remote one) that this code violates
	//someone's intellectual property rights. If this concerns you feel free to
	//contact me for an alternative, though less efficient, implementation.
	
	void computeGradients(float kernelRadius, int kernelWidth) {
		
		//generate the gaussian convolution masks
		float *kernel = new float[kernelWidth];
		float *diffKernel = new float[kernelWidth];
		int kwidth;
		for (kwidth = 0; kwidth < kernelWidth; kwidth++) {
			float g1 = gaussian(kwidth, kernelRadius);
			if (g1 <= GAUSSIAN_CUT_OFF && kwidth >= 2) break;
			float g2 = gaussian(kwidth - 0.5, kernelRadius);
			float g3 = gaussian(kwidth + 0.5, kernelRadius);
			kernel[kwidth] = (g1 + g2 + g3) / 3.0 / (2.0 * (float) M_PI * kernelRadius * kernelRadius);
			diffKernel[kwidth] = g3 - g2;
		}

		int initX = kwidth - 1;
		int maxX = width - (kwidth - 1);
		int initY = width * (kwidth - 1);
		int maxY = width * (height - (kwidth - 1));
		
		//perform convolution in x and y directions
		for (int x = initX; x < maxX; x++) {
			for (int y = initY; y < maxY; y += width) {
				int index = x + y;
				float sumX = data[index] * kernel[0];
				float sumY = sumX;
				int xOffset = 1;
				int yOffset = width;
				for(; xOffset < kwidth ;) {
					sumY += kernel[xOffset] * (data[index - yOffset] + data[index + yOffset]);
					sumX += kernel[xOffset] * (data[index - xOffset] + data[index + xOffset]);
					yOffset += width;
					xOffset++;
				}
				
				yConv[index] = sumY;
				xConv[index] = sumX;
			}
 
		}
 
		for (int x = initX; x < maxX; x++) {
			for (int y = initY; y < maxY; y += width) {
				float sum = 0.0;
				int index = x + y;
				for (int i = 1; i < kwidth; i++)
					sum += diffKernel[i] * (yConv[index - i] - yConv[index + i]);
 
				xGradient[index] = sum;
			}
 
		}

		for (int x = kwidth; x < width - kwidth; x++) {
			for (int y = initY; y < maxY; y += width) {
				float sum = 0.0;
				int index = x + y;
				int yOffset = width;
				for (int i = 1; i < kwidth; i++) {
					sum += diffKernel[i] * (xConv[index - yOffset] - xConv[index + yOffset]);
					yOffset += width;
				}
 
				yGradient[index] = sum;
			}
 
		}
 
		initX = kwidth;
		maxX = width - kwidth;
		initY = width * kwidth;
		maxY = width * (height - kwidth);
		for (int x = initX; x < maxX; x++) {
			for (int y = initY; y < maxY; y += width) {
				int index = x + y;
				int indexN = index - width;
				int indexS = index + width;
				int indexW = index - 1;
				int indexE = index + 1;
				int indexNW = indexN - 1;
				int indexNE = indexN + 1;
				int indexSW = indexS - 1;
				int indexSE = indexS + 1;
				
				float xGrad = xGradient[index];
				float yGrad = yGradient[index];
				float gradMag = hypot(xGrad, yGrad);

				//perform non-maximal supression
				float nMag = hypot(xGradient[indexN], yGradient[indexN]);
				float sMag = hypot(xGradient[indexS], yGradient[indexS]);
				float wMag = hypot(xGradient[indexW], yGradient[indexW]);
				float eMag = hypot(xGradient[indexE], yGradient[indexE]);
				float neMag = hypot(xGradient[indexNE], yGradient[indexNE]);
				float seMag = hypot(xGradient[indexSE], yGradient[indexSE]);
				float swMag = hypot(xGradient[indexSW], yGradient[indexSW]);
				float nwMag = hypot(xGradient[indexNW], yGradient[indexNW]);
				float tmp;
				/*
				 * An explanation of what's happening here, for those who want
				 * to understand the source: This performs the "non-maximal
				 * supression" phase of the Canny edge detection in which we
				 * need to compare the gradient magnitude to that in the
				 * direction of the gradient; only if the value is a local
				 * maximum do we consider the point as an edge candidate.
				 * 
				 * We need to break the comparison into a number of different
				 * cases depending on the gradient direction so that the
				 * appropriate values can be used. To avoid computing the
				 * gradient direction, we use two simple comparisons: first we
				 * check that the partial derivatives have the same sign (1)
				 * and then we check which is larger (2). As a consequence, we
				 * have reduced the problem to one of four identical cases that
				 * each test the central gradient magnitude against the values at
				 * two points with 'identical support'; what this means is that
				 * the geometry required to accurately interpolate the magnitude
				 * of gradient function at those points has an identical
				 * geometry (upto right-angled-rotation/reflection).
				 * 
				 * When comparing the central gradient to the two interpolated
				 * values, we avoid performing any divisions by multiplying both
				 * sides of each inequality by the greater of the two partial
				 * derivatives. The common comparand is stored in a temporary
				 * variable (3) and reused in the mirror case (4).
				 * 
				 */
				if (xGrad * yGrad <= (float) 0 /*(1)*/
					? abs(xGrad) >= abs(yGrad) /*(2)*/
						? (tmp = abs(xGrad * gradMag)) >= abs(yGrad * neMag - (xGrad + yGrad) * eMag) /*(3)*/
							&& tmp > abs(yGrad * swMag - (xGrad + yGrad) * wMag) /*(4)*/
						: (tmp = abs(yGrad * gradMag)) >= abs(xGrad * neMag - (yGrad + xGrad) * nMag) /*(3)*/
							&& tmp > abs(xGrad * swMag - (yGrad + xGrad) * sMag) /*(4)*/
					: abs(xGrad) >= abs(yGrad) /*(2)*/
						? (tmp = abs(xGrad * gradMag)) >= abs(yGrad * seMag + (xGrad - yGrad) * eMag) /*(3)*/
							&& tmp > abs(yGrad * nwMag + (xGrad - yGrad) * wMag) /*(4)*/
						: (tmp = abs(yGrad * gradMag)) >= abs(xGrad * seMag + (yGrad - xGrad) * sMag) /*(3)*/
							&& tmp > abs(xGrad * nwMag + (yGrad - xGrad) * nMag) /*(4)*/
					) {
					magnitude[index] = gradMag >= MAGNITUDE_LIMIT ? MAGNITUDE_MAX : (int) (MAGNITUDE_SCALE * gradMag);
					//NOTE: The orientation of the edge is not employed by this
					//implementation. It is a simple matter to compute it at
					//this point as: atan2(yGrad, xGrad);
				} else {
					magnitude[index] = 0;
				}
			}
		}
	}
 
	//NOTE: It is quite feasible to replace the implementation of this method
	//with one which only loosely approximates the hypot function. I've tested
	//simple approximations such as abs(x) + abs(y) and they work fine.
	//float hypot(float x, float y) {
		//return (float) hypot(x, y);
	//}
 
	float gaussian(float x, float sigma) {
		return (float) exp(-(x * x) / (2.0 * sigma * sigma));
	}
 
	void performHysteresis(int low, int high) {
		//NOTE: this implementation reuses the data array to store both
		//luminance data from the image, and edge intensity from the processing.
		//This is done for memory efficiency, other implementations may wish
		//to separate these functions.
		for (int i=0; i < picsize; i++)
			data[i] = 0;
 
		int offset = 0;
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				if (data[offset] == 0 && magnitude[offset] >= high) {
					follow(x, y, offset, low);
				}
				offset++;
			}
		}
 	}
 
	void follow(int x1, int y1, int i1, int threshold) {
		int x0 = x1 == 0 ? x1 : x1 - 1;
		int x2 = x1 == width - 1 ? x1 : x1 + 1;
		int y0 = y1 == 0 ? y1 : y1 - 1;
		int y2 = y1 == height -1 ? y1 : y1 + 1;
		
		data[i1] = magnitude[i1];
		for (int x = x0; x <= x2; x++) {
			for (int y = y0; y <= y2; y++) {
				int i2 = x + y * width;
				if ((y != y1 || x != x1)
					&& data[i2] == 0 
					&& magnitude[i2] >= threshold) {
					follow(x, y, i2, threshold);
					return;
				}
			}
		}
	}

	void thresholdEdges() {
		for (int i = 0; i < picsize; i++) {
			//data[i] = data[i] > 0 ? -1 : 0xff000000;
			data[i] = data[i] > 0 ? 0xff000000: -1;
		}
	}
	
	int luminance(float r, float g, float b) {
		return round(0.299f * r + 0.587f * g + 0.114f * b);
	}
	
	void readLuminance() {
			int* pixels = (int*) sourceImage->imagebits;
			for (int i = 0; i < picsize; i++) {
				int p = pixels[i];		// RGBA image = 0xAABBGGRR int
				int b = (p & 0xff0000) >> 16;
				int g = (p & 0xff00) >> 8;
				int r = p & 0xff;
				data[i] = luminance(r, g, b);
			}
#if 0
		int type = sourceImage.getType();
		if (type == BufferedImage.TYPE_INT_RGB || type == BufferedImage.TYPE_INT_ARGB) {
		} else if (type == BufferedImage.TYPE_BYTE_GRAY) {
			byte* pixels = (byte*) sourceImage.getData().getDataElements(0, 0, width, height, null);
			for (int i = 0; i < picsize; i++) {
				data[i] = (pixels[i] & 0xff);
			}
		} else if (type == BufferedImage.TYPE_USHORT_GRAY) {
			short* pixels = (short*) sourceImage.getData().getDataElements(0, 0, width, height, null);
			for (int i = 0; i < picsize; i++) {
				data[i] = (pixels[i] & 0xffff) / 256;
			}
		} else if (type == BufferedImage.TYPE_3BYTE_BGR) {
            byte* pixels = (byte*) sourceImage.getData().getDataElements(0, 0, width, height, null);
            int offset = 0;
            for (int i = 0; i < picsize; i++) {
                int b = pixels[offset++] & 0xff;
                int g = pixels[offset++] & 0xff;
                int r = pixels[offset++] & 0xff;
                data[i] = luminance(r, g, b);
            }
        } else {
			throw new IllegalArgumentException("Unsupported image type: " + type);
		}
#endif
	}
 
	void normalizeContrast() {
		int* histogram = new int[256];
		for (int i = 0; i < picsize; i++) {
			histogram[data[i]]++;
		}
		int* remap = new int[256];
		int sum = 0;
		int j = 0;
		for (int i = 0; i < 256; i++) {
			sum += histogram[i];
			int target = sum*255/picsize;
			for (int k = j+1; k <=target; k++) {
				remap[k] = i;
			}
			j = target;
		}
		
		for (int i = 0; i < picsize; i++) {
			data[i] = remap[data[i]];
		}
	}
	
	void writeEdges(int *pixels) {
		//if (edgesImage == 0) {
			edgesImage = new MWIMAGEHDR;
			edgesImage->flags = 0;
			edgesImage->width = width;
			edgesImage->height = height;
			edgesImage->planes = 1;
			edgesImage->bpp = 32;
			edgesImage->data_format = MWIF_RGBA8888;
			edgesImage->pitch = width * 4;
			edgesImage->imagebits = (MWUCHAR *)new int[picsize];
			edgesImage->palette = 0;
			edgesImage->palsize = 0;
			edgesImage->transcolor = MWNOCOLOR;
			memcpy(edgesImage->imagebits, pixels, picsize << 2);
		//}
	}
};

PMWIMAGEHDR
CannyEdgeDetect(PMWIMAGEHDR pimage)
{
	CannyEdgeDetector *detector = new CannyEdgeDetector();

	detector->setLowThreshold(2.5);
	detector->setHighThreshold(7.5);
	detector->setSourceImage(pimage);
	detector->process();

	PMWIMAGEHDR newimage = detector->getEdgesImage();
	return newimage;
}
