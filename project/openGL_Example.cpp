// openGL_Test.cpp : Defines the entry point for the console application.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "../include/GL/glew.h"
#include "../include/GL/freeglut.h"

#include "../include/assimp/Importer.hpp"
#include "../include/assimp/scene.h"
#include "../include/assimp/postprocess.h"

#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "../include/glm/gtc/type_ptr.hpp"

#include "../include/opencv/opencv2/opencv.hpp"

#if defined(_WIN32)
#pragma comment(lib,"../lib/x64/freeglut.lib")
#pragma comment(lib,"../lib/x64/glew32.lib")
#pragma comment(lib,"../lib/x64/glm_static.lib")
#pragma comment(lib,"../lib/x64/opencv_world410.lib")
#pragma comment(lib,"../lib/x64/assimp-vc140-mt.lib")
#endif


#define WINDOW_TITLE_PREFIX "Press WASD to move camera, Press QE to adjust focus"

using namespace cv;

int CurrentWidth = 800,	CurrentHeight = 600,	WindowHandle = 0;
int degree=0;

const aiScene* scene;

unsigned FrameCount = 0;

void ReshapeFunction(int, int);
void RenderFunction(void);
void recursive_render(const aiScene* sc, const aiNode* nd);

int focalAdjust = 0;
int move_x = 0;
int move_y = 0;
std::vector<Mat> blurImg(255);	//算出不同程度的模糊圖像
bool blurFlag = true;	//每次都計算模糊太花時間 //使用flag當視角改變時再模糊
void keyboardFunction(unsigned char key, int x, int y)
{
	//使用按鍵A與M調整角度
	switch (key)
	{
	case 'q':
		focalAdjust++;
		break;
	case 'e':
		focalAdjust--;
		break;
	case 'w':
		move_y+=10;
		blurFlag = true;
		break;
	case 's':
		move_y-=10;
		blurFlag = true;
		break;
	case 'a':
		move_x-=10;
		blurFlag = true;
		break;
	case 'd':
		move_x+=10;
		blurFlag = true;
		break;
	}
	RenderFunction();
}

int main(int argc, char* argv[])
{
	//初始化 glut
	glutInit(&argc, argv);

	//以下使用 Context 功能
	/*	glutInitContextVersion(4, 0);
		glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
		glutInitContextProfile(GLUT_CORE_PROFILE);
		glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);*/

	//設定 glut 畫布尺寸 與color / depth模式
	glutInitWindowSize(CurrentWidth, CurrentHeight);
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA);
	
	//根據已設定好的 glut (如尺寸,color,depth) 向window要求建立一個視窗，接著若失敗則退出程式
	WindowHandle = glutCreateWindow(WINDOW_TITLE_PREFIX);
	if(WindowHandle < 1) {	fprintf(stderr,"ERROR: Could not create a new rendering window.\n");exit(EXIT_FAILURE);	}
	
	glutReshapeFunc(ReshapeFunction); //設定視窗 大小若改變，則跳到"AResizeFunction"這個函數處理應變事項
	glutDisplayFunc(RenderFunction);  //設定視窗 如果要畫圖 則執行"RenderFunction"
	//glutIdleFunc(IdleFunction);		  //閒置時...請系統執行"IdleFunction"
	glutKeyboardFunc(keyboardFunction);

	GLenum GlewInitResult = glewInit();
	if (GlewInitResult != GLEW_OK ) {	fprintf(stderr,"ERROR: %s\n",glewGetErrorString(GlewInitResult)	);	exit(EXIT_FAILURE);	}

	//背景顏色
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
	
	//透過Assimp讀檔
	Assimp::Importer importer;
	scene = importer.ReadFile("Knight.ply", aiProcess_Triangulate | aiProcess_FlipUVs);

	glutMainLoop();
	
	exit(EXIT_SUCCESS);
}


void ReshapeFunction(int Width, int Height)
{
	CurrentWidth = Width;
	CurrentHeight = Height;
	glViewport(0, 0, CurrentWidth, CurrentHeight);
}

void RenderFunction(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, CurrentWidth, CurrentHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(-CurrentWidth / 50.0, CurrentWidth / 50.0,
		-CurrentHeight / 50.0, CurrentHeight / 50.0,
		50, 50000);

	gluLookAt(move_x, -600+ move_y, 80, move_x, move_y, 80, 0, 0, 1);	//相機移動
	
	glMatrixMode(GL_MODELVIEW);
	
	glPushMatrix();
		glPushMatrix();
		glTranslatef(0, 0, 0);
		recursive_render(scene, scene->mRootNode);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(30, -100, 0);
		recursive_render(scene, scene->mRootNode);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(60, -200, 0);
		recursive_render(scene, scene->mRootNode);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-30, 100, 0);
		recursive_render(scene, scene->mRootNode);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(-60, 200, 0);
		recursive_render(scene, scene->mRootNode);
		glPopMatrix();
	glPopMatrix();

	glFlush();
	
	Mat ColorImg(CurrentHeight,CurrentWidth,CV_8UC3);
	Mat ColorImgDump(CurrentHeight, CurrentWidth, CV_8UC3);
	Mat DepthImg(CurrentHeight, CurrentWidth, CV_8U);
	Mat DepthImgFloat(CurrentHeight, CurrentWidth, CV_32F);
	Mat DepthImgDump(CurrentHeight, CurrentWidth, CV_32F);
	
	glReadPixels(0,0, CurrentWidth, CurrentHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, ColorImgDump.data); //讀取彩色影像
	flip(ColorImgDump, ColorImg,0);

	glReadPixels(0, 0, CurrentWidth, CurrentHeight, GL_DEPTH_COMPONENT, GL_FLOAT, DepthImgDump.data);//讀取深度
	flip(DepthImgDump, DepthImgFloat, 0);
	DepthImgFloat.convertTo(DepthImg, CV_8U, 255.0  , 0);//將深度從0-1轉為0-255

	glutSwapBuffers();

	int back = 0;
	int front = 255;
	for(int i=0;i<DepthImg.rows;i++)	//取出最大和最小的深度
		for (int j = 0; j < DepthImg.cols; j++)
		{
			int depth = DepthImg.at<uchar>(i, j);
			if (depth < front)
				front = depth;
			else if (depth > back&& depth != 255)
				back = depth;
		}

	int focalLength = (front + back) / 2;	//算出中間值

	focalLength += focalAdjust;	//加上調整深度的參數

	if (focalLength > back)//避免超出焦距
	{
		focalAdjust = back - (focalLength - focalAdjust);
		focalLength = back;
	}
	else if (focalLength < front)
	{
		focalAdjust = front - (focalLength - focalAdjust);
		focalLength = front;
	}

	printf("now focal length: %d\n", focalLength);

	//printf("%d %d\n", back, front);
	if (blurFlag == true)
	{
		ColorImg.copyTo(blurImg[0]);
		for (int i = 1; i < blurImg.size(); i++)
			blur(ColorImg, blurImg[i], Size(i, i));
		blurFlag = false;
	}

	std::vector<std::vector<Point2f>> depthMap(back - front + 1);
	Mat resultImg;
	ColorImg.copyTo(resultImg);

	for (int i = 0; i < ColorImg.rows; i++)
		for (int j = 0; j < ColorImg.cols; j++)
		{
			int depth = DepthImg.at<uchar>(i, j);
			if (depth == 255)	//無東西的值是255所以忽略不算
				continue;
			else
			{
				int blurLevel = abs(depth - focalLength);	//計算與當前焦距的距離
				depthMap[back-depth].push_back(Point2f(i, j));
			}
		}

	for (int i = 0; i < depthMap.size(); i++)	// 0的話與當前焦距相同不處理//所以從1開始
		for (int j = 0; j < depthMap[i].size(); j++)
		{
			int depth = back - i;
			int blurLevel = abs(depth - focalLength);
			if (depthMap[i][j].x - blurLevel >= 0 && depthMap[i][j].y - blurLevel >= 0 && depthMap[i][j].x + blurLevel < resultImg.rows && depthMap[i][j].y + blurLevel < resultImg.cols)	//防止超出圖像大小
				for (int k = -blurLevel / 2; k <= blurLevel / 2; k++)
					for (int l = -blurLevel / 2; l <= blurLevel / 2; l++)
					{
						int px = depthMap[i][j].x + k;
						int py = depthMap[i][j].y + l;
						resultImg.at<Vec3b>(px, py) = blurImg[blurLevel].at<Vec3b>(px, py);	//根據算出的距離取不同程度的模糊圖像
					}
		}

	imshow("Focus Map", resultImg);	//使用opencv顯示出具有焦距的圖像
	waitKey(10);
	//glutPostRedisplay();
}

void recursive_render(const aiScene* sc, const aiNode* nd)
{
	unsigned int i;
	unsigned int n = 0, t;
	aiMatrix4x4 m = nd->mTransformation;

	// update transform
	aiMatrix4x4 mT = m.Transpose();
	glMultMatrixf((float*)&mT);
	glPushMatrix();


	// draw all meshes assigned to this node
	for (n = 0; n < nd->mNumMeshes; n++) {
		const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];


		for (t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch (face->mNumIndices) {
			case 1: face_mode = GL_POINTS; break;
			case 2: face_mode = GL_LINES; break;
			case 3: face_mode = GL_TRIANGLES; break;
			default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);
			for (i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				if (mesh->mColors[0] != NULL) glColor4fv((GLfloat*)&mesh->mColors[0][index]);
				if (mesh->mNormals != NULL)
				{
					glNormal3fv(&mesh->mNormals[index].x);
				}

				glVertex3fv(&mesh->mVertices[index].x);
			}

			glEnd();
		}

	}

	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n) {
		recursive_render(sc, nd->mChildren[n]);
	}

	glPopMatrix();
}