#include <opencv2/opencv.hpp>
#include <stdio.h>
#pragma warning(disable:4996)
#include <stdlib.h>
#include <time.h>

typedef struct Stroke {
	CvPoint P;
	CvScalar C;
}Strk;

void paint(IplImage* src, IplImage* dst, int R[], int flag);
void paintLayer(IplImage* dst, IplImage* ref, int R, int flag);
void shuffleArray(Strk* S, int n);
int getBri(CvScalar f);
void makeSplineStroke(int x0, int y0, int R, IplImage* ref, IplImage* dst);

int main(void) {

	char file[1000];
	int method;

	printf("21011746 Yang Hyunseok\n");
	printf("Multimedia Programming Assignment #4\n\n");

	IplImage* src;

	//	validation for file path
	while (1) {
		printf("Input File Path : ");
		scanf("%s", file);
		getchar();
		src = cvLoadImage(file);
		if (src != NULL) {
			break;
		}
		else {
			printf("File not Found!\n");
		}
	}

	//	validation for method number
	while (1) {
		printf("Select Drawing Mode (0=circle, 1=stroke) : ");
		scanf("%d", &method);
		if (method == 1 || method == 0) {
			break;
		}
		else {
			printf("Wrong Drawing Mode!\n");
		}
	}

	CvSize size = cvGetSize(src);
	int h = size.height;
	int w = size.width;

	//	canvas
	IplImage* dst = cvCreateImage(size, 8, 3);
	cvSet(dst, cvScalar(255, 255, 255));

	//	brush size (odd num)
	int R[5] = { 31,19,11,5,3 };

	cvShowImage("src", src);

	paint(src, dst, R, method);

	cvShowImage("dst", dst);
	cvWaitKey();

	return 0;
}

// Functions

void paint(IplImage* src, IplImage* dst, int R[], int flag) {

	IplImage* ref = cvCreateImage(cvGetSize(src), 8, 3);
	int kernel;

	for (int i = 0; i < 5; i++) {
		//	kernel은 반드시 odd number이어야 한다.
		kernel = R[i] * 3;

		cvSmooth(src, ref, CV_GAUSSIAN, kernel);
		cvShowImage("dst", dst);
		cvWaitKey(1000);

		//	사실 논문에 의하면 R[i]와 kernel은 비례해야 한다.
		//	하지만 시행착오 과정에서 Brush size가 2일 때 더 정교한 그림이 나오기에 수정하였다.
		if (R[i] == 3 && flag == 1) {
			R[i] = 2;
		}
		paintLayer(dst, ref, R[i], flag);
	}
}

void paintLayer(IplImage* dst, IplImage* ref, int R, int flag) {

	// Stroke 배열 생성 (색상, 위치)
	Strk* S;

	int idx = 0;

	int h = cvGetSize(ref).height;
	int w = cvGetSize(ref).width;

	// 그리드 한 변의 길이 gridX, girdY
	int divX = w / R;
	int divY = h / R;
	int gridX = w / divX;
	int gridY = h / divY;

	//	int S_size = divX * divY;
	//	메모리 효율을 위해 위처럼 사이즈를 정하려 했으나, 상수 값으로 고정시킴.
	int S_size = 10000000;

	S = (Strk*)malloc(sizeof(Strk) * S_size);


	for (int y = 0; y < h; y += gridY) {
		for (int x = 0; x < w; x += gridX) {

			//	err : T와 비교하기 위한 전체 평균 변수, sum_err : Max_ERR를 구하기 위한 변수
			float err = 0.f;
			int ct = 0;
			int m_x = 0;
			int m_y = 0;

			float sum_err;
			float Max_ERR = 0.f;

			// 평균 에러 구하기
			for (int v = 0; v < gridY; v++) {
				for (int u = 0; u < gridX; u++) {

					//	initialization
					sum_err = 0;

					if (x + u < 0 || x + u > w - 1) continue;
					if (y + v < 0 || y + v > h - 1) continue;

					//	canvas와 ref의 에러 값 구하기
					CvScalar f = cvGet2D(dst, y + v, x + u);
					CvScalar g = cvGet2D(ref, y + v, x + u);

					for (int k = 0; k < 3; k++) {
						err += abs(f.val[k] - g.val[k]);
						sum_err += abs(f.val[k] - g.val[k]);
					}

					if (Max_ERR < sum_err) {
						Max_ERR = sum_err;
						m_x = x + u;
						m_y = y + v;
					}
					ct++;
				}
			}

			//	ct로 카운팅 후 평균 내기
			err /= (float)ct;

			//	임계값 T는 임의로 지정 -> 변경 가능 -> Thresholding의 원리
			if (err > 30) {
				S[idx].C = cvGet2D(ref, m_y, m_x);
				S[idx].P = cvPoint(m_x, m_y);
				idx++;

				//	만일 배열의 크기를 벗어나게 될 경우 반복문을 탈출한다.
				if (idx >= S_size - 1) { break; }
			}
		}
	}


	if (flag == 0) {
		//	paint all strokes in S on the canvas in random order
		shuffleArray(S, idx);

		for (int i = 0; i < idx; i++) {
			cvCircle(dst, S[i].P, R, S[i].C, -1);
		}
	}

	else {
		//	paint all strokes in S on the canvas in random order
		//	항상 Circle이 그려지는 지점이 spline의 시작이다.
		shuffleArray(S, idx);

		for (int i = 0; i < idx; i++) {
			makeSplineStroke(S[i].P.x, S[i].P.y, R, ref, dst);
		}
	}

	free(S);
}

void makeSplineStroke(int x0, int y0, int R, IplImage* ref, IplImage* dst) {
	CvScalar strokeColor = cvGet2D(ref, y0, x0);

	const int maxStrokeLength = 50;
	int minStrokeLength = 7;
	int idx = 1;

	Stroke K[maxStrokeLength];

	//	first element(에러 값이 가장 컸던 첫 지점 - 처음 점을 찍은 곳)
	K[0].C = strokeColor;
	K[0].P = cvPoint(x0, y0);


	int lastDx = 0, lastDy = 0;

	int x = x0, y = y0;

	//	논문에는 나와있지만, 교수님이 안해도 된다고 한 것.
	float fc = 0.5f;

	for (int i = 1; i < maxStrokeLength; i++) {

		if (i > minStrokeLength && (abs(getBri(cvGet2D(ref, y, x)) - getBri(cvGet2D(dst, y, x))) < abs(getBri(cvGet2D(ref, y, x)) - getBri(strokeColor)))) {
			break;
		}

		//	gradient를 구할 범위
		int n = 1;

		//	범위를 벗어날 때
		if (x + n > ref->width - 1 || x - n < 0 || y + n > ref->height - 1 || y - n < 0) continue;

		//	a vector of gradient
		float gx = getBri(cvGet2D(ref, y, x + n)) - getBri(cvGet2D(ref, y, x - n));
		float gy = getBri(cvGet2D(ref, y + n, x)) - getBri(cvGet2D(ref, y - n, x));


		//	색의 차이가 없다!
		if (gx == 0 && gy == 0) {
			break;
		}

		//	수직 방향의 벡터
		float dx = -gy;
		float dy = gx;

		//	reverse direction
		if (lastDx * dx + lastDy * dy <= 0) {
			dx = -dx;
			dy = -dy;
		}

		//	빼도 되는 부분이지만 테스트로 해보았습니다.
		dx = fc * dx + (1 - fc) * lastDx;
		dy = fc * dy + (1 - fc) * lastDy;

		// 벡터의 크기
		float L = sqrt(dx * dx + dy * dy);

		// 크기를 1로 변경
		dx /= L;
		dy /= L;

		// R만큼 이동 (반지름)
		x = x + R * dx;
		y = y + R * dy;

		lastDx = dx;
		lastDy = dy;

		if (x > ref->width - 1 || x < 0 || y > ref->height - 1 || y < 0) break;

		K[idx].P = cvPoint(x, y);
		K[idx].C = strokeColor;
		idx++;
	}


	for (int i = 0; i < idx - 1; i++) {
		cvLine(dst, K[i].P, K[i + 1].P, strokeColor, R);
	}

}

int getBri(CvScalar f) {
	return (f.val[0] + f.val[1] + f.val[2]) / 3;
}

void shuffleArray(Strk* S, int n) {

	for (int i = n - 1; i > 0; i--) {
		int j = rand() % (i + 1);

		Strk temp = S[i];
		S[i] = S[j];
		S[j] = temp;
	}
}