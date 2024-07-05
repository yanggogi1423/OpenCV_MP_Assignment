#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include <stdio.h>
#pragma warning(disable:4996)

void myFastestMeanFilter(IplImage* src, IplImage* dst, int K);

int main() {

	char file_path[1000];

	printf("Input File Path : ");
	scanf("%s", file_path);
	getchar();

	IplImage* src = cvLoadImage(file_path);
	CvSize size = cvGetSize(src);
	int w = size.width;
	int h = size.height;

	IplImage* dst = cvCreateImage(size, 8, 3);


	int K = 50;
	printf("Input Kernel Size : ");
	scanf("%d", &K);

	myFastestMeanFilter(src, dst, K);

	cvShowImage("src", src);
	cvShowImage("dst", dst);
	cvWaitKey();

	return 0;
}

void myFastestMeanFilter(IplImage* src, IplImage* dst, int K) {
	CvSize size = cvGetSize(src);
	int w = size.width;
	int h = size.height;


	CvScalar** arr;

	//	Memory Allocation.
	arr = (CvScalar**)malloc(sizeof(CvScalar*) * h);

	for (int i = 0; i < h; i++) {
		arr[i] = (CvScalar*)malloc(sizeof(CvScalar) * w);
	}

	//	mySetArray(arr, h, w);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			CvScalar f = cvGet2D(src, y, x);

			if (x == 0) {	//	y == 0 일때에도 포함

				if (y == 0) {	// x==0 && y==0
					for (int k = 0; k < 3; k++) {
						arr[y][x].val[k] = f.val[k];
					}
				}

				else {
					for (int k = 0; k < 3; k++) {
						arr[y][x].val[k] = f.val[k] + arr[y - 1][x].val[k];
					}
				}

			}

			else if (y == 0) {
				//	이미 x, y == 0 인 경우를 해결함.
				for (int k = 0; k < 3; k++) {
					arr[y][x].val[k] = f.val[k] + arr[y][x - 1].val[k];
				}
			}

			else {			//	y>0 && x>0

				for (int k = 0; k < 3; k++) {
					arr[y][x].val[k] = f.val[k] + arr[y - 1][x].val[k] + arr[y][x - 1].val[k] - arr[y - 1][x - 1].val[k];
				}
			}
		}
	}

	

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			CvScalar f;
			int tempY1 = y + K;
			int tempY2 = y - K;
			int tempX1 = x + K;
			int tempX2 = x - K;

			if (tempY1 >= h) {
				tempY1 = h - 1;
			}
			if (tempY2 < 0) {
				tempY2 = 0;
			}
			if (tempX1 >= w) {
				tempX1 = w - 1;
			}
			if (tempX2 < 0) {
				tempX2 = 0;
			}

			for (int k = 0; k < 3; k++) {
				f.val[k] = arr[tempY1][tempX1].val[k] - arr[tempY2][tempX1].val[k] - arr[tempY1][tempX2].val[k] + arr[tempY2][tempX2].val[k];
				f.val[k] /= ((tempY1 - tempY2 + 1) * (tempX1 - tempX2 + 1));
			}

			cvSet2D(dst, y, x, f);
			
		}
	}


	// free memory

	for (int i = 0; i < h; i++) {
		free(arr[i]);
	}

	free(arr);
}
