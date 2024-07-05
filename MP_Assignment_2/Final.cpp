#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <time.h>

int main(void) {

	char name[1000];

	printf("Test CV\nInput File Name : ");
	scanf("%s", name);

	//  Timer ON
	int TIME = 0;
	clock_t start = clock();

	//	원본 이미지 불러오기
	IplImage* src = cvLoadImage(name);
	CvSize size = cvGetSize(src);
	int w = size.width;
	int h = size.height;

	//	B, G, R 
	IplImage* div[3], * dst = cvCreateImage(cvSize(w, h / 3), 8, 3);
	for (int i = 0; i < 3; i++)
		div[i] = cvCreateImage(cvSize(w, h / 3), 8, 3);


	//	위에서부터 1/3씩 이미지를 불러와서 div[]에 저장한다.
	for (int i = 0; i < 3; i++) {
		for (int y = i * (h / 3); y < (i + 1) * (h / 3); y++) {
			for (int x = 0; x < w; x++) {
				CvScalar temp = cvGet2D(src, y, x);
				cvSet2D(div[i], y - i * (h / 3), x, temp);
			}
		}
	}

	//	주의 사항 : div -> (0, 1), (0, 2)를 비교하는 것이 아닌 0, 2를 각각 1과 비교할 것임. (이유는 보고서에 기술)
	//	Compare - div[0], div[1]
	int min_u1 = 0;
	int min_v1 = 0;
	float min_err1 = FLT_MAX;

	//	Compare - div[1], div[2]
	int min_u2 = 0;
	int min_v2 = 0;
	float min_err2 = FLT_MAX;

	//	그리드를 몇 줄로 나눌 지 정하는 N, 그리고 가운데의 한 조각을 알기 위한 n, m 선언
	int N = 5;
	int n = N / 2;
	int m = N / 2 + 1;

	//	속도를 올리기 위하여 u, v값의 범위를 줄여주기 위한 M을 선언
	int M = 25;

	//	또한 속도를 올리기 위하여 확인하는 x, y를 줄여주는 plus를 선언
	int plus = 5;

	//	SSD를 통한 비교 시작
	for (int v = (-h / 3) / M; v <= (h / 3) / M; v += 1) {	//	u, v를 x, y처럼 건너뛰지 않는 이유는 정확도가 확연하게 떨어질 수 있기 때문이다.
		for (int u = -w / M; u <= w / M; u += 1) {
			//	err1, err2 - 차잇값 저장, ct - count
			float err1 = 0.0f;
			float err2 = 0.0f;
			int ct = 0;
			for (int y = (h / 3) / N * n; y < (h / 3) / N * m; y += plus) {		// 11*11칸(N*N)으로 나눈다.
				for (int x = w / N * n; x < w / N * m; x += plus) {
					int x1 = x;
					int y1 = y;
					int x2 = x + u;
					int y2 = y + v;
					int x3 = x + u;
					int y3 = y + v;

					//	범위를 벗어나면 ERROR가 발생하기에 막아준다.
					if (x2 < 0 || x2 > w - 1) continue;
					if (y2 < 0 || y2 > h / 3 - 1) continue;
					if (x3 < 0 || x3 > w - 1) continue;
					if (y3 < 0 || y3 > h / 3 - 1) continue;

					CvScalar f1 = cvGet2D(div[0], y2, x2);
					CvScalar f2 = cvGet2D(div[1], y1, x1);
					CvScalar f3 = cvGet2D(div[2], y3, x3);

					err1 += (f1.val[2] - f2.val[2]) * (f1.val[2] - f2.val[2]);		//	해당되지 않는 다른 색상을 비교하는게 합당하고 생각했음
					err2 += (f2.val[0] - f3.val[0]) * (f2.val[0] - f3.val[0]);

					ct++;
				}
			}

			//	Means
			err1 /= ct;
			err2 /= ct;

			//	min_err1 최신화
			if (err1 < min_err1) {
				min_err1 = err1;
				min_u1 = u;
				min_v1 = v;
			}

			//	min_err2 최신화
			if (err2 < min_err2) {
				min_err2 = err2;
				min_u2 = u;
				min_v2 = v;
			}
		}
	}


	for (int y = 0; y < h / 3; y++) {
		for (int x = 0; x < w; x++) {

			//	범위를 벗어나면 ERROR가 발생하기에 막아준다.
			if (y + min_v1 < 0 || y + min_v1 > h / 3 - 1)continue;
			if (x + min_u1 < 0 || x + min_u1 > w - 1) continue;
			if (y + min_v2 < 0 || y + min_v2 > h / 3 - 1)continue;
			if (x + min_u2 < 0 || x + min_u2 > w - 1) continue;

			CvScalar f1 = cvGet2D(div[1], y, x);
			CvScalar f2 = cvGet2D(div[0], y + min_v1, x + min_u1);
			CvScalar f3 = cvGet2D(div[2], y + min_v2, x + min_u2);

			//	div[0] - BLUE, div[1] - GREEN, div[2] - RED
			CvScalar g;
			g.val[1] = f1.val[1];
			g.val[0] = f2.val[0];
			g.val[2] = f3.val[2];

			cvSet2D(dst, y, x, g);
		}
	}

	//	Timer OFF
	TIME += ((int)clock() - start) / (CLOCKS_PER_SEC / 1000);

	cvShowImage("src", src);
	cvShowImage("dst", dst);

	printf("TIME : %d ms\n", TIME);

	cvWaitKey();

	return 0;
}

