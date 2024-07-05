#include <opencv2/opencv.hpp>
#include "MatrixInverse.h"
#include "vec.h"
#include "mat.h"

//	Source Image
IplImage* src = nullptr;
IplImage* dst = nullptr;
int W = 500;										// 결과 이미지 가로 크기
int H = 500;										// 결과 이미지 세로 크기

//	MATRIX 함수들

//	내적 함수
void multiplyMatrix(float M[][8], float A[][8], float B[][8]) {
	for (int i = 0; i < 8; i++) {
		M[0][i] = 0.0f;
		for (int j = 0; j < 8; j++) {
			M[0][i] += A[i][j] * B[0][j];
		}
	}
}

//	1*8 행렬을 3*3 행렬로 변환하기
void convertMatrix(float M[][8],float dstM[][3]) {
	float Mat[3][3];

	int idx = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (idx == 8) {	//	H33을 넣는다.
				Mat[i][j] = 1;
				break;
			}
			Mat[i][j] = M[0][idx++];
		}
	}

	InverseMatrixGJ3(Mat, dstM);
}

//	Homography를 하기 위해서 8개의 변수(H33 제외)를 찾는다.
void estimateTransform(float M[][3], CvPoint P[], CvPoint Q[]) {

	float A[8][8] = { 0 };
	float B[1][8] = { 0 };

	//	행렬 A, B에 값 대입 -> int의 성질을 이용(i/2 -> floor)
	for (int i = 0; i < 8; i++) {
		if (i % 2 == 0) {	//	index가 짝수일 때(0 포함)
			A[i][0] = -P[i / 2].x;
			A[i][1] = -P[i / 2].y;
			A[i][2] = -1;
			A[i][6] = Q[i / 2].x * P[i / 2].x;
			A[i][7] = Q[i / 2].x * P[i / 2].y;

			B[0][i] = -Q[i / 2].x;
		}
		else {		//	index가 홀수일 때
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

//	역변형으로 출력
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


vec3 pos[8] = {										// 육면체를 구성하는 8개의 꼭지점 3차원 좌표
		vec3(-0.5, -0.5,  0.5),
		vec3(-0.5,  0.5,  0.5),
		vec3(0.5,  0.5,  0.5),
		vec3(0.5, -0.5,  0.5),
		vec3(-0.5, -0.5, -0.5),
		vec3(-0.5,  0.5, -0.5),
		vec3(0.5,  0.5, -0.5),
		vec3(0.5, -0.5, -0.5) };



struct rect											// 사각형 한 면
{
	int ind[4];										// 꼭지점의 인덱스
	vec3 pos[4];									// 꼭지점의 화면 방향으로의 3차원 위치
	vec3 nor;										// 법선(normal) 벡터 방향 (= 면이 향하는 방향)
};

rect setRect(int a, int b, int c, int d)			// 사각형 정보를 채워주는 함수(바로 아래 cube정의에 사용)
{
	rect r;
	r.ind[0] = a;
	r.ind[1] = b;
	r.ind[2] = c;
	r.ind[3] = d;
	return r;
}

rect cube[6] = { setRect(1, 0, 3, 2),				// 사각형 6개를 정의해 육면체를 구성
				 setRect(2, 3, 7, 6),
				 setRect(3, 0, 4, 7),
				 setRect(6, 5, 1, 2),
				 setRect(6, 7, 4, 5),
				 setRect(5, 4, 0, 1) };

vec3 epos = vec3(1.5, 1.5, 1.5);					// 카메라(시점의 3차원) 위치
mat4 ModelMat;										// 모델에 변형을 주는 변형 행렬
mat4 ViewMat;										// 카메라 시점을 맞춰주는 변형 행렬
mat4 ProjMat;										// 화면상 위치로 투영해주는 변형 행렬

void init()											// 초기화
{
	ModelMat = mat4(1.0f);
	ViewMat = LookAt(epos, vec3(0, 0, 0), vec3(0, 1, 0));
	// 카메라 위치(epos)에서 (0,0,0)을 바라보는 카메라 설정			
	ProjMat = Perspective(45, W / (float)H, 0.1, 100);
	// 45도의 시야각을 가진 투영 변환 (가시거리 0.1~100)
}

void rotateModel(float rx, float ry, float rz)		// 육면체 모델에 회전을 적용하는 함수
{
	ModelMat = RotateX(rx) * RotateY(ry) * RotateZ(rz) * ModelMat;
}

vec3 convert3Dto2D(vec3 in)							// 3차원 좌표를 화면에 투영된 2차원+깊이값(z) 좌표로 변환
{
	vec4 p = ProjMat * ViewMat * ModelMat * vec4(in);
	p.x /= p.w;
	p.y /= p.w;
	p.z /= p.w;
	p.x = (p.x + 1) / 2.0f * W;
	p.y = (-p.y + 1) / 2.0f * H;
	return vec3(p.x, p.y, p.z);
}

void updatePosAndNormal(rect* r, vec3 p[])			// 육면체의 회전에 따른 각 면의 3차원 좌표 및 법선 벡터 방향 업데이트
{
	for (int i = 0; i < 4; i++)
		r->pos[i] = convert3Dto2D(p[r->ind[i]]);
	vec3 a = normalize(r->pos[0] - r->pos[1]);
	vec3 b = normalize(r->pos[2] - r->pos[1]);
	r->nor = cross(a, b);
}


void drawImage()									// 그림을 그린다 (각 면의 테두리를 직선으로 그림)
{
	cvSet(dst, cvScalar(0, 0, 0));
	for (int i = 0; i < 6; i++)
	{
		updatePosAndNormal(&cube[i], pos);
		if (cube[i].nor.z < 0) continue;			// 보이지 않는 사각형을 제외, 보이는 사각형만 그린다	

		//	dst의 움직이는 점
		CvPoint Q[4];

		//	0,0에서 반시계 방향으로
		CvPoint SRC[4] = { cvPoint(0,0), cvPoint(0,src->height - 1),
			cvPoint(src->width - 1,src->height - 1), cvPoint(src->width - 1,0)
		};

		//	역변형을 위한 3*3 행렬
		float IM[3][3];

		for (int j = 0; j < 4; j++)
		{
			vec3 p1 = cube[i].pos[j];
			vec3 p2 = cube[i].pos[(j + 1) % 4];

			//	배열 Q에 각 지점의 좌표를 저장
			Q[j].x = p1.x;
			Q[j].y = p1.y;

			cvLine(dst, cvPoint(p1.x, p1.y), cvPoint(p2.x, p2.y), cvScalar(255, 255, 255), 3);
		}

		//	행렬 구하고
		estimateTransform(IM, SRC, Q);
		//	적용한다
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
		rotateModel(dy, dx, -dy);					// 마우스 조작에 따라 모델을 회전함
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

	//	파일 path를 제대로 입력해야 실행된다.
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