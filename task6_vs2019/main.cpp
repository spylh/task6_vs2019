#include <stdio.h>
#include <Windows.h>
#include <GL/GL.h>
#include <GL/glut.h>

#include <HD/hd.h>
#include <HDU/hduError.h>
#include <HDU/hduMatrix.h>


hduVector3Dd gCenterOfStylusSphere, gCenterOfGodSphere, gForce;
HHD ghHD = HD_INVALID_HANDLE;
HDSchedulerHandle gSchedulerCallback = HD_INVALID_HANDLE;

void resize(int w, int h) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-100.0, 100.0, -100.0, 100.0, -100.0, 100.0);
}

void doGraphicsState() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glEnable(GL_SMOOTH);

	GLfloat lightZeroPosition[] = { 10.0f,4.0f,100.0f,0.0f };
	GLfloat lightZeroColor[] = { 0.6f,0.6f,0.6f,1.0f };
	GLfloat lightOnePosition[] = { -1.0f,-2.0f,-100.0f,0.0f };
	GLfloat lightOneColor[] = { 0.6f,0.6f,0.6f,1.0f };

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
	glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOneColor);

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	glEnable(GL_DEPTH_TEST);
}

void idle(void) {
	glutPostRedisplay();
	if (!hdWaitForCompletion(gSchedulerCallback, HD_WAIT_CHECK_STATUS)) {
		printf("メインスケジューラが終了しました。\n何かキーを押すと終了します。");
		getchar();
		exit(-1);
	}
}

#define SSR 5
#define BOARDSIZE 60
#define STIFFNESS 0.30

#define RES 0.1 //頂点を配置する細かさ

int point_index;
int loop_counter;
GLdouble line_points[30000][4];

void updateEffectorPosition(void) {
	int buttonState;
	double m_currentDistance;
	hduVector3Dd m_centerToEffector = gCenterOfStylusSphere;
	gCenterOfGodSphere = gCenterOfStylusSphere;

	//黒板の範囲内で押し込んだ場合
	if (m_centerToEffector[2] < SSR && abs(m_centerToEffector[0]) < BOARDSIZE / 2 && abs(m_centerToEffector[1]) < BOARDSIZE / 2) {
		//力覚フィードバック
		gCenterOfGodSphere[2] = SSR;
		m_currentDistance = SSR - m_centerToEffector[2];
		gForce.set(0.0, 0.0, STIFFNESS * m_currentDistance);
		//お絵描き
		hdGetIntegerv(HD_CURRENT_BUTTONS, &buttonState);
		if (buttonState == 1) {
			//添字リセット
			if (point_index >= 30000)
			{
				point_index = 0;
				loop_counter = 29999;
			}
			else {
				++point_index;
				if(loop_counter  != 29999)loop_counter = point_index;
			}
			line_points[point_index][0] = (GLdouble)gCenterOfGodSphere[0];
			line_points[point_index][1] = (GLdouble)gCenterOfGodSphere[1];
			line_points[point_index][2] = SSR + 0.01;
			line_points[point_index][3] = 1;
		}
		else if (buttonState == 2){
			for (int i = 0; i < loop_counter; i++) {
				if (sqrt(pow(line_points[i][0] - gCenterOfGodSphere[0], 2) + pow(line_points[i][1] - gCenterOfGodSphere[1], 2)) < SSR) {
					line_points[i][3] = 0;
				}
			}

		}

	}
	//そうでない場合
	else {
		gForce.set(0.0, 0.0, 0.0);
	}

}

HDCallbackCode HDCALLBACK ContactCB(void* data) {
	HHD hHD = hdGetCurrentDevice();

	hdBeginFrame(hHD);

	hdGetDoublev(HD_CURRENT_POSITION, gCenterOfStylusSphere);
	updateEffectorPosition();
	hdSetDoublev(HD_CURRENT_FORCE, gForce);

	hdEndFrame(hHD);
	return HD_CALLBACK_CONTINUE;
}

void display() {

	doGraphicsState();

	glPushMatrix();
	glTranslated(0.0,0.0,-BOARDSIZE/2);
	glColor4d(0.2, 0.8, 0.8, 1.0);
	glutSolidCube(BOARDSIZE);
	glPopMatrix();

	glPushMatrix();
	glTranslated(gCenterOfGodSphere[0], gCenterOfGodSphere[1], gCenterOfGodSphere[2]);
	glColor4d(0.9, 0.1, 0.1, 1.0);
	glutSolidSphere(SSR, 20, 20);
	glPopMatrix();

	glPushMatrix();
	glTranslated(gCenterOfStylusSphere[0], gCenterOfStylusSphere[1], gCenterOfStylusSphere[2]);
	glColor4d(1.0, 1.0, 0.0, 1.0);
	glutWireSphere(SSR, 20, 20);
	glPopMatrix();

	glColor4d(0.0, 0.0, 0.0, 1.0);
	if (point_index > -1) {
		for (int i = 0; i < loop_counter; i++) {
			glBegin(GL_POINTS);
			if(line_points[i][3]>0)glVertex3d(line_points[i][0], line_points[i][1], line_points[i][2]);
			glEnd();
		}
	}

	glutSwapBuffers();
}


void exitHandler() {
	hdStopScheduler();

	if (ghHD != HD_INVALID_HANDLE) {
		hdDisableDevice(ghHD);
		ghHD = HD_INVALID_HANDLE;
	}

	printf("終了\n");
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'q')exit(0);
}

int main(int argc, char* argv[]) {
	HDErrorInfo error;

	atexit(exitHandler);

	ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
	hdEnable(HD_FORCE_OUTPUT);
	hdEnable(HD_MAX_FORCE_CLAMPING);
	hdStartScheduler();

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutCreateWindow("hellohaptics");
	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);

	gSchedulerCallback = hdScheduleAsynchronous(ContactCB, NULL, HD_DEFAULT_SCHEDULER_PRIORITY);
	glutMainLoop();

	return 0;
}
