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

	//	���� �̹��� �ҷ�����
	IplImage* src = cvLoadImage(name);
	CvSize size = cvGetSize(src);
	int w = size.width;
	int h = size.height;

	//	B, G, R 
	IplImage* div[3], * dst = cvCreateImage(cvSize(w, h / 3), 8, 3);
	for (int i = 0; i < 3; i++)
		div[i] = cvCreateImage(cvSize(w, h / 3), 8, 3);


	//	���������� 1/3�� �̹����� �ҷ��ͼ� div[]�� �����Ѵ�.
	for (int i = 0; i < 3; i++) {
		for (int y = i * (h / 3); y < (i + 1) * (h / 3); y++) {
			for (int x = 0; x < w; x++) {
				CvScalar temp = cvGet2D(src, y, x);
				cvSet2D(div[i], y - i * (h / 3), x, temp);
			}
		}
	}

	//	���� ���� : div -> (0, 1), (0, 2)�� ���ϴ� ���� �ƴ� 0, 2�� ���� 1�� ���� ����. (������ ������ ���)
	//	Compare - div[0], div[1]
	int min_u1 = 0;
	int min_v1 = 0;
	float min_err1 = FLT_MAX;

	//	Compare - div[1], div[2]
	int min_u2 = 0;
	int min_v2 = 0;
	float min_err2 = FLT_MAX;

	//	�׸��带 �� �ٷ� ���� �� ���ϴ� N, �׸��� ����� �� ������ �˱� ���� n, m ����
	int N = 5;
	int n = N / 2;
	int m = N / 2 + 1;

	//	�ӵ��� �ø��� ���Ͽ� u, v���� ������ �ٿ��ֱ� ���� M�� ����
	int M = 25;

	//	���� �ӵ��� �ø��� ���Ͽ� Ȯ���ϴ� x, y�� �ٿ��ִ� plus�� ����
	int plus = 5;

	//	SSD�� ���� �� ����
	for (int v = (-h / 3) / M; v <= (h / 3) / M; v += 1) {	//	u, v�� x, yó�� �ǳʶ��� �ʴ� ������ ��Ȯ���� Ȯ���ϰ� ������ �� �ֱ� �����̴�.
		for (int u = -w / M; u <= w / M; u += 1) {
			//	err1, err2 - ���հ� ����, ct - count
			float err1 = 0.0f;
			float err2 = 0.0f;
			int ct = 0;
			for (int y = (h / 3) / N * n; y < (h / 3) / N * m; y += plus) {		// 11*11ĭ(N*N)���� ������.
				for (int x = w / N * n; x < w / N * m; x += plus) {
					int x1 = x;
					int y1 = y;
					int x2 = x + u;
					int y2 = y + v;
					int x3 = x + u;
					int y3 = y + v;

					//	������ ����� ERROR�� �߻��ϱ⿡ �����ش�.
					if (x2 < 0 || x2 > w - 1) continue;
					if (y2 < 0 || y2 > h / 3 - 1) continue;
					if (x3 < 0 || x3 > w - 1) continue;
					if (y3 < 0 || y3 > h / 3 - 1) continue;

					CvScalar f1 = cvGet2D(div[0], y2, x2);
					CvScalar f2 = cvGet2D(div[1], y1, x1);
					CvScalar f3 = cvGet2D(div[2], y3, x3);

					err1 += (f1.val[2] - f2.val[2]) * (f1.val[2] - f2.val[2]);		//	�ش���� �ʴ� �ٸ� ������ ���ϴ°� �մ��ϰ� ��������
					err2 += (f2.val[0] - f3.val[0]) * (f2.val[0] - f3.val[0]);

					ct++;
				}
			}

			//	Means
			err1 /= ct;
			err2 /= ct;

			//	min_err1 �ֽ�ȭ
			if (err1 < min_err1) {
				min_err1 = err1;
				min_u1 = u;
				min_v1 = v;
			}

			//	min_err2 �ֽ�ȭ
			if (err2 < min_err2) {
				min_err2 = err2;
				min_u2 = u;
				min_v2 = v;
			}
		}
	}


	for (int y = 0; y < h / 3; y++) {
		for (int x = 0; x < w; x++) {

			//	������ ����� ERROR�� �߻��ϱ⿡ �����ش�.
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

