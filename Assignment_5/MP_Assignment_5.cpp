#include <opencv2/opencv.hpp>
#include "MatrixInverse.h"
#include "vec.h"
#include "mat.h"

//	Source Image
IplImage* src = nullptr;
IplImage* dst = nullptr;
int W = 500;										// ��� �̹��� ���� ũ��
int H = 500;										// ��� �̹��� ���� ũ��

//	MATRIX �Լ���

//	���� �Լ�
void multiplyMatrix(float M[][8], float A[][8], float B[][8]) {
	for (int i = 0; i < 8; i++) {
		M[0][i] = 0.0f;
		for (int j = 0; j < 8; j++) {
			M[0][i] += A[i][j] * B[0][j];
		}
	}
}

//	1*8 ����� 3*3 ��ķ� ��ȯ�ϱ�
void convertMatrix(float M[][8],float dstM[][3]) {
	float Mat[3][3];

	int idx = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (idx == 8) {	//	H33�� �ִ´�.
				Mat[i][j] = 1;
				break;
			}
			Mat[i][j] = M[0][idx++];
		}
	}

	InverseMatrixGJ3(Mat, dstM);
}

//	Homography�� �ϱ� ���ؼ� 8���� ����(H33 ����)�� ã�´�.
void estimateTransform(float M[][3], CvPoint P[], CvPoint Q[]) {

	float A[8][8] = { 0 };
	float B[1][8] = { 0 };

	//	��� A, B�� �� ���� -> int�� ������ �̿�(i/2 -> floor)
	for (int i = 0; i < 8; i++) {
		if (i % 2 == 0) {	//	index�� ¦���� ��(0 ����)
			A[i][0] = -P[i / 2].x;
			A[i][1] = -P[i / 2].y;
			A[i][2] = -1;
			A[i][6] = Q[i / 2].x * P[i / 2].x;
			A[i][7] = Q[i / 2].x * P[i / 2].y;

			B[0][i] = -Q[i / 2].x;
		}
		else {		//	index�� Ȧ���� ��
			A[i][3] = -P[i / 2].x;
			A[i][4] = -P[i / 2].y;
			A[i][5] = -1;
			A[i][6] = Q[i / 2].y * P[i / 2].x;
			A[i][7] = Q[i / 2].y * P[i / 2].y;

			B[0][i] = -Q[i / 2].y;
		}
	}

	float invA[8][8];
	float temp[1][8];
	InverseMatrixGJ8(A, invA);
	multiplyMatrix(temp, invA, B);

	convertMatrix(temp, M);
}

//	���������� ���
void applyInverseTransform(IplImage* src, IplImage* dst, float M[][3]) {

	for (int y2 = 0; y2 < dst->height; y2++) {
		for (int x2 = 0; x2 < dst->width; x2++) {
			float w2 = 1;

			float x1 = M[0][0] * x2 + M[0][1] * y2 + M[0][2] * w2;
			float y1 = M[1][0] * x2 + M[1][1] * y2 + M[1][2] * w2;
			float w1 = M[2][0] * x2 + M[2][1] * y2 + M[2][2] * w2;
			x1 /= w1;
			y1 /= w1;

			if (x1<0 || x1>src->width - 1) continue;
			if (y1<0 || y1>src->height - 1) continue;
			CvScalar f = cvGet2D(src, y1, x1);
			cvSet2D(dst, y2, x2, f);

		}
	}
}


vec3 pos[8] = {										// ����ü�� �����ϴ� 8���� ������ 3���� ��ǥ
		vec3(-0.5, -0.5,  0.5),
		vec3(-0.5,  0.5,  0.5),
		vec3(0.5,  0.5,  0.5),
		vec3(0.5, -0.5,  0.5),
		vec3(-0.5, -0.5, -0.5),
		vec3(-0.5,  0.5, -0.5),
		vec3(0.5,  0.5, -0.5),
		vec3(0.5, -0.5, -0.5) };



struct rect											// �簢�� �� ��
{
	int ind[4];										// �������� �ε���
	vec3 pos[4];									// �������� ȭ�� ���������� 3���� ��ġ
	vec3 nor;										// ����(normal) ���� ���� (= ���� ���ϴ� ����)
};

rect setRect(int a, int b, int c, int d)			// �簢�� ������ ä���ִ� �Լ�(�ٷ� �Ʒ� cube���ǿ� ���)
{
	rect r;
	r.ind[0] = a;
	r.ind[1] = b;
	r.ind[2] = c;
	r.ind[3] = d;
	return r;
}

rect cube[6] = { setRect(1, 0, 3, 2),				// �簢�� 6���� ������ ����ü�� ����
				 setRect(2, 3, 7, 6),
				 setRect(3, 0, 4, 7),
				 setRect(6, 5, 1, 2),
				 setRect(6, 7, 4, 5),
				 setRect(5, 4, 0, 1) };

vec3 epos = vec3(1.5, 1.5, 1.5);					// ī�޶�(������ 3����) ��ġ
mat4 ModelMat;										// �𵨿� ������ �ִ� ���� ���
mat4 ViewMat;										// ī�޶� ������ �����ִ� ���� ���
mat4 ProjMat;										// ȭ��� ��ġ�� �������ִ� ���� ���

void init()											// �ʱ�ȭ
{
	ModelMat = mat4(1.0f);
	ViewMat = LookAt(epos, vec3(0, 0, 0), vec3(0, 1, 0));
	// ī�޶� ��ġ(epos)���� (0,0,0)�� �ٶ󺸴� ī�޶� ����			
	ProjMat = Perspective(45, W / (float)H, 0.1, 100);
	// 45���� �þ߰��� ���� ���� ��ȯ (���ðŸ� 0.1~100)
}

void rotateModel(float rx, float ry, float rz)		// ����ü �𵨿� ȸ���� �����ϴ� �Լ�
{
	ModelMat = RotateX(rx) * RotateY(ry) * RotateZ(rz) * ModelMat;
}

vec3 convert3Dto2D(vec3 in)							// 3���� ��ǥ�� ȭ�鿡 ������ 2����+���̰�(z) ��ǥ�� ��ȯ
{
	vec4 p = ProjMat * ViewMat * ModelMat * vec4(in);
	p.x /= p.w;
	p.y /= p.w;
	p.z /= p.w;
	p.x = (p.x + 1) / 2.0f * W;
	p.y = (-p.y + 1) / 2.0f * H;
	return vec3(p.x, p.y, p.z);
}

void updatePosAndNormal(rect* r, vec3 p[])			// ����ü�� ȸ���� ���� �� ���� 3���� ��ǥ �� ���� ���� ���� ������Ʈ
{
	for (int i = 0; i < 4; i++)
		r->pos[i] = convert3Dto2D(p[r->ind[i]]);
	vec3 a = normalize(r->pos[0] - r->pos[1]);
	vec3 b = normalize(r->pos[2] - r->pos[1]);
	r->nor = cross(a, b);
}


void drawImage()									// �׸��� �׸��� (�� ���� �׵θ��� �������� �׸�)
{
	cvSet(dst, cvScalar(0, 0, 0));
	for (int i = 0; i < 6; i++)
	{
		updatePosAndNormal(&cube[i], pos);
		if (cube[i].nor.z < 0) continue;			// ������ �ʴ� �簢���� ����, ���̴� �簢���� �׸���	

		//	dst�� �����̴� ��
		CvPoint Q[4];

		//	0,0���� �ݽð� ��������
		CvPoint SRC[4] = { cvPoint(0,0), cvPoint(0,src->height - 1),
			cvPoint(src->width - 1,src->height - 1), cvPoint(src->width - 1,0)
		};

		//	�������� ���� 3*3 ���
		float IM[3][3];

		for (int j = 0; j < 4; j++)
		{
			vec3 p1 = cube[i].pos[j];
			vec3 p2 = cube[i].pos[(j + 1) % 4];

			//	�迭 Q�� �� ������ ��ǥ�� ����
			Q[j].x = p1.x;
			Q[j].y = p1.y;

			cvLine(dst, cvPoint(p1.x, p1.y), cvPoint(p2.x, p2.y), cvScalar(255, 255, 255), 3);
		}

		//	��� ���ϰ�
		estimateTransform(IM, SRC, Q);
		//	�����Ѵ�
		applyInverseTransform(src, dst, IM);
	}
	cvShowImage("3D view", dst);
}

void myMouse(int event, int x, int y, int flags, void*)
{
	static CvPoint prev = cvPoint(0, 0);
	if (event == CV_EVENT_LBUTTONDOWN)
		prev = cvPoint(x, y);
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON) == CV_EVENT_FLAG_LBUTTON)
	{
		int dx = x - prev.x;
		int dy = y - prev.y;
		rotateModel(dy, dx, -dy);					// ���콺 ���ۿ� ���� ���� ȸ����
		drawImage();
		prev = cvPoint(x, y);
	}
}

int main()
{

	char str[1000];

	printf("21011746 Yang Hyunseok\n");
	printf("Multimedia Programming Assignment #5\n");
	printf("=====================================\n\n");

	//	���� path�� ����� �Է��ؾ� ����ȴ�.
	while (1) {
		printf("Input file path : ");
		scanf("%s", str);
		getchar();

		src = cvLoadImage(str);

		if (src != nullptr) {
			break;
		}
		printf("File not found!\n");
	}


	dst = cvCreateImage(cvSize(W, H), 8, 3);
	init();

	while (true)
	{
		rotateModel(0, 1, 0);
		drawImage();
		cvSetMouseCallback("3D view", myMouse);
		int key = cvWaitKey(1);
		if (key == ' ') key = cvWaitKey();
	}

	return 0;
}