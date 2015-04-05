#include <opencv2/opencv.hpp>
#include <stdio.h>

#define  min(a,b)  ((a<b)?a:b)
#define  max(a,b)  ((a>b)?a:b)

	/************************************************************************/
	/*                     YUV12 to RGB24   (4)	Basic Optimizations	 3.1	*/
	/************************************************************************/
	int  yuv420_to_argb888(const unsigned char *y, const unsigned char *u,const unsigned char *v ,int width ,int height ,unsigned char *rgb)
	{
		static const int Precision = 32768 ;
		static const int Coefficient_y =  (int) (1.164*Precision + 0.5);
		static const int Coefficient_rv = (int) (1.596*Precision + 0.5);
		static const int Coefficient_gu = (int) (0.391*Precision + 0.5);
		static const int Coefficient_gv = (int) (0.813*Precision + 0.5);
		static const int Coefficient_bu = (int) (2.018*Precision + 0.5);
		static  int CoefficientY[256];
		static  int CoefficientRV[256];
		static  int CoefficientGU[256];
		static  int CoefficientGV[256];
		static  int CoefficientBU[256];
		static  int _CoefficientsR[1024];
		//static   int _CoefficientsG[1024];
		//static   int _CoefficientsB[1024];
		static int flag = 1 ;

		if (flag)
		{

			for(int i =0;i<256;i++)
			{
				CoefficientY[i] = Coefficient_y *(i - 16) + (Precision/2);
				CoefficientGV[i] = -Coefficient_gv *(i - 128);
				CoefficientBU[i] = Coefficient_bu *(i - 128);
				CoefficientGU[i] = -Coefficient_gu *(i - 128);
				CoefficientRV[i] = Coefficient_rv *(i - 128);
			}

			for(int j=0;j<1024;j++)
			{
				_CoefficientsR[j] = min((max(j-320,0)),255) ;
				//_CoefficientsG[j] = min((max(j-320,0)),255) ;
				//_CoefficientsB[j] = min((max(j-320,0)),255) ;
			}

			flag = 0;
		}
		CoefficientY[0] = -593888;
		CoefficientY[1] = -555746;                            //修复bug!! CoefficientY[1]在第二次进入此函数的时候意外被修改为很大的数,理论值应该为-555746
		int *CoefficientsR = &_CoefficientsR[320];
		// int *CoefficientsG = &_CoefficientsG[320];
		// int *CoefficientsB = &_CoefficientsB[320];


		for ( int h=0;h<height;h++)
		{

				for (int w=0;w<width;w++)
				{
					int k = h*width + w;
					int index = k*3;
					int i = (h/2)*(width/2)+(w/2);
					int Y = y[k];
					int U = u[i];
					int V = v[i];

					
					//3.3 Optimizations Removing Conditional Tests
					int r = CoefficientY[Y] + CoefficientRV[V];
					int g = CoefficientY[Y] + CoefficientGU[U]+ CoefficientGV[V];
					int b = CoefficientY[Y] + CoefficientBU[U];
					rgb[index]   = CoefficientsR[r/Precision];
					rgb[index+1] = CoefficientsR[g/Precision];
					rgb[index+2] = CoefficientsR[b/Precision];
				
					
				}
		}



		return 0;
	}




//Compare 3 images histograms together, 
// the first is divided in half along y to test its other half
// Call is: 
//    ch7HistCmp modelImage0 testImage1 testImage2 badImage3
// Note that the model image is split in half.  Top half(0) makes model.  It's then tested
// against its lower half(0), testImages 1 and 2 in different lighting and different object 3
// 
int main( int argc, char** argv ) {

    IplImage* src[5], *tmp;
	int i;
		if((src[0] = cvLoadImage(argv[1], 1)) == 0){ //We're going to split this one in half
			printf("Error on reading image 1, %s\n",argv[1]);
			return(-1);
		}
		//Parse the first image into two image halves divided halfway on y
		printf("Getting size [[%d] [%d]]  format is [%s]\n",src[0]->width,src[0]->height,src[0]->channelSeq);
		CvSize size = cvGetSize(src[0]);
		printf("Get size %d %d\n",size.width,size.height);
		int width = size.width;
		int height = size.height;
		int halfheight = height >> 1;

		unsigned char * rgb = (unsigned char*)src[0]->imageData;
		unsigned char * yuv = (unsigned char*)malloc(width*height + width*height/2);
		unsigned char * y = yuv;
		unsigned char * u = &yuv[width*height];
		unsigned char * v = &yuv[width*height+width*height/4];

		int k = 0;
		for(int i = 0;i<height ;i++)
		{
			for(int j =0;j<width;j++)
			{
				int index = i*width+j;
				unsigned char R = 	rgb[width*i*3+j*3];
				unsigned char G = 	rgb[width*i*3+j*3+1];
				unsigned char B = 	rgb[width*i*3+j*3+2];
				y[index] =  ( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16;
				y[index] =  min(max(y[index],0),255);

				if( (j%2 ==0)&&(i%2 == 0) )
				{
					k++;
					u[k] =  ( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
					v[k] =  ( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128;
					u[k] =  min(max(u[k],0),255);
					v[k] =  min(max(v[k],0),255);
				}
			}
		}


		src[1] = cvCreateImage(cvSize(width,height), 8, 3);
		yuv420_to_argb888((const unsigned char*)y,(const unsigned char*)u,(const unsigned char*)v,width,height,(unsigned char *)src[1]->imageData);
		
        //DISPLAY
		cvNamedWindow( "Source0", 1 );
        cvShowImage(   "Source0", src[0] );
		cvNamedWindow( "Source1", 1 );
        cvShowImage(   "Source1", src[1] );

        cvWaitKey(0);

		cvReleaseImage(&src[0]);
		cvReleaseImage(&src[1]);

		free(yuv);

}
