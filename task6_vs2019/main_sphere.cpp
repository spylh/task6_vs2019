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
	GLfloat lightZeroColor[] = { 1.0f,1.0f,1.0f,1.0f };
	GLfloat lightOnePosition[] = { -1.0f,-2.0f,-100.0f,0.0f };
	GLfloat lightOneColor[] = { 0.6f,0.6f,0.6f,1.0f };

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
	glLightfv(GL_LIGHT0, GL_POSITION, lightOnePosition);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightOneColor);

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

#define SSR 20.0
#define FSR 60.0
#define STIFFNESS 0.10


#define EPSILON 0.00001

void updateEffectorPosition(void) {
	double m_currentDistance;
	hduVector3Dd m_centerToEffector = gCenterOfStylusSphere;

	m_currentDistance = m_centerToEffector.magnitude();

	if (m_currentDistance > SSR + FSR) {
		gCenterOfGodSphere = gCenterOfStylusSphere;
		gForce.set(0.0, 0.0, 0.0);
	}
	else if (m_currentDistance > EPSILON) {
		double scale = (SSR + FSR) / m_currentDistance;
		gCenterOfGodSphere = scale * m_centerToEffector;
		gForce = STIFFNESS * (gCenterOfGodSphere - gCenterOfStylusSphere);
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

	glDisable(GL_CULL_FACE);

	glPushMatrix();
	glTranslated(0.0, 0.0, 0.0);
	glColor4d(0.2, 0.8, 0.8, 1.0);
	glutSolidSphere(FSR, 20, 20);
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
