#define _CRT_SECURE_NO_WARNINGS

#ifdef _MSC_VER
#	pragma comment(lib, "opengl32.lib")  // Compiler-specific directive to avoid manually configuration
#  pragma comment(lib, "glu32.lib")     // Link libraries
#endif


// Ustalanie trybu tekstowego:
#ifdef _MSC_VER        // Check if MS Visual C compiler
#   ifndef _MBCS
#      define _MBCS    // Uses Multi-byte character set
#   endif
#   ifdef _UNICODE     // Not using Unicode character set
#      undef _UNICODE 
#   endif
#   ifdef UNICODE
#      undef UNICODE 
#   endif
#endif

#include <windows.h>            // Window defines
#include <gl\gl.h>              // OpenGL
#include <gl\glu.h>             // GLU library
#include <math.h>				// Define for sqrt
#include <stdio.h>
#include "resource.h"			// About box resource identifiers.

#define glRGB(x, y, z)	glColor3ub((GLubyte)x, (GLubyte)y, (GLubyte)z)
#define BITMAP_ID 0x4D42		// identyfikator formatu BMP
#define GL_PI 3.14159265359f

// Color Palette handle
HPALETTE hPalette = NULL;

// Application name and instance storeage
static const TCHAR lpszAppName[] = TEXT("GL Lazik grafika");
static HINSTANCE hInstance;

// Rotation amounts
static GLfloat xRot = 0.0f;
static GLfloat yRot = 0.0f;


static GLsizei lastHeight;
static GLsizei lastWidth;

// --- Camera Variables for Orbiting/Zooming/Panning ---
static GLfloat cameraDistance = 200.0f;
static GLfloat cameraAngleX = 0.0f; 
static GLfloat cameraAngleY = 0.0f;

// Pan/Translate variables
static GLfloat cameraPanX = 0.0f;
static GLfloat cameraPanY = 0.0f;

// Mouse state for interaction
static int lastMouseX;
static int lastMouseY;
static BOOL isFirstMouse = TRUE;
static BOOL isPanning = FALSE;
static BOOL isCursorLocked = TRUE;

static BOOL isResettingCamera = FALSE;
static ULONGLONG resetStartTime = 0;
// --- End New Camera Variables ---

// --- Tilt Variables ---
static GLfloat steeringAngle = 0.0f;
static GLfloat pitchAngle = 0.0f;

static const GLfloat maxSteeringAngle = 10.0f;
static const GLfloat maxPitchAngle = 10.0f;
static const GLfloat tiltSmoothing = 0.1f;

static const GLfloat steeringSpeed = 7.0f;
static GLfloat currentWheelSpeed = 0.0f;

static GLfloat decorationRotationAngle = 0.0f;
// --- End Tilt Variables ---

// --- UFO Movement Variables ---
struct UFOPosition {
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

static UFOPosition ufoPos = { 0.0f, 0.0f, 0.0f };

static GLfloat ufoVelX = 0.0f;
static GLfloat ufoVelY = 0.0f;
static GLfloat ufoVelZ = 0.0f;

static const GLfloat acceleration = 0.15f;
static const GLfloat friction = 0.96f;
static const GLfloat ufoMaxSpeed = 4.0f;
static GLfloat ufoMinHeight = 10.0f;
// --- End UFO Movement Variables ---

static BOOL isDecorationsSpinning = TRUE;
static BOOL keyA_held = FALSE;
static BOOL keyD_held = FALSE;
static BOOL keyW_held = FALSE;
static BOOL keyS_held = FALSE;
static BOOL keySpacja_held = FALSE;
static BOOL keyCTRL_held = FALSE;

// Opis tekstury
BITMAPINFOHEADER	bitmapInfoHeader;	// nag��wek obrazu
unsigned char* bitmapData = NULL;			// dane tekstury
unsigned int		texture[2];			// obiekt tekstury


// Declaration for Window procedure
LRESULT CALLBACK WndProc(HWND    hWnd,
	UINT    message,
	WPARAM  wParam,
	LPARAM  lParam);

// Dialog procedure for about box
BOOL APIENTRY AboutDlgProc(HWND hDlg, UINT message, UINT wParam, LONG lParam);

// Set Pixel Format function - forward declaration
void SetDCPixelFormat(HDC hDC);


// Change viewing volume and viewport.  Called when window is resized
void ChangeSize(GLsizei w, GLsizei h) {
	GLfloat nRange = 100.0f;
	GLfloat fAspect;
	// Prevent a divide by zero
	if (h == 0)
		h = 1;

	lastWidth = w;
	lastHeight = h;

	fAspect = (GLfloat)w / (GLfloat)h;
	// Set Viewport to window dimensions
	glViewport(0, 0, w, h);

	// Reset coordinate system
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Establish perspective: (FOV, Aspect, Near, Far)
	gluPerspective(60.0f, fAspect, 1.0, 15000.0); // Use perspective

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


// This function does any needed initialization on the rendering context.  Here it sets up and initializes the lighting for the scene.
void SetupRC()
{
	glEnable(GL_DEPTH_TEST);	// Hidden surface removal
	glFrontFace(GL_CCW);		// Counter clock-wise polygons face out

	// White background
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Black brush
	glColor3f(0.0, 0.0, 0.0);
}


unsigned char* LoadBitmapFile(const char* filename, BITMAPINFOHEADER* bitmapInfoHeader)
{
	FILE* filePtr;							// wskaźnik pozycji pliku
	BITMAPFILEHEADER	bitmapFileHeader;	// nagłówek pliku
	unsigned char* bitmapImage = NULL;	// dane obrazu
	int					imageIdx = 0;		// licznik pikseli
	unsigned char		tempRGB;			// zmienna zamiany składowych

	// otwiera plik w trybie "read binary"
	// FIX: Use "rb"
	filePtr = fopen(filename, "rb");
	if (filePtr == NULL)
	{
		return NULL;
	}

	// wczytuje nagłówek pliku
	if (fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr) != 1)
	{
		fclose(filePtr);
		return NULL;
	}

	// sprawdza, czy jest to plik formatu BMP
	if (bitmapFileHeader.bfType != BITMAP_ID)
	{
		fclose(filePtr);
		return NULL;
	}

	// wczytuje nagłówek obrazu
	if (fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr) != 1)
	{
		fclose(filePtr);
		return NULL;
	}

	// FIX: If biSizeImage is 0, calculate it (common in older BMPs)
	if (bitmapInfoHeader->biSizeImage == 0)
	{
		bitmapInfoHeader->biSizeImage = bitmapInfoHeader->biWidth * bitmapInfoHeader->biHeight * 3;
	}

	// ustawia wskaźnik pozycji pliku na początku danych obrazu
	if (fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET) != 0)
	{
		fclose(filePtr);
		return NULL;
	}

	// przydziela pamięć buforowi obrazu
	bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);

	// sprawdza, czy udało się przydzielić pamięć
	if (!bitmapImage)
	{
		// FIX: Was: free(bitmapImage); - which is wrong if bitmapImage is NULL.
		fclose(filePtr);
		return NULL;
	}

	// wczytuje dane obrazu
	if (fread(bitmapImage, 1, bitmapInfoHeader->biSizeImage, filePtr) != bitmapInfoHeader->biSizeImage)
	{
		// FIX: Error while reading data
		free(bitmapImage);
		fclose(filePtr);
		return NULL;
	}

	// zamienia miejscami składowe R i B (B G R -> R G B)
	// This loop is correct for 24-bit (3 bytes per pixel) RGB format data.
	for (imageIdx = 0; imageIdx < (int)bitmapInfoHeader->biSizeImage; imageIdx += 3)
	{
		tempRGB = bitmapImage[imageIdx];
		bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
		bitmapImage[imageIdx + 2] = tempRGB;
	}

	// zamyka plik i zwraca wskaźnik bufora zawierającego wczytany obraz
	fclose(filePtr);
	return bitmapImage;
}

GLuint base;

GLvoid BuildFont(HDC hDC) {
	HFONT hFont;
	base = glGenLists(96);

	hFont = CreateFont(-14,
		0,
		0,
		0,
		FW_BOLD,
		FALSE,
		FALSE,
		FALSE,
		ANSI_CHARSET,
		OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		FF_DONTCARE | DEFAULT_PITCH,
		"Courier New");

	SelectObject(hDC, hFont);
	wglUseFontBitmaps(hDC, 32, 96, base);
	DeleteObject(hFont);
}

GLvoid KillFont(GLvoid) {
	glDeleteLists(base, 96);
}

GLvoid glPrint(const char* fmt, ...) {
	char text[256];
	va_list ap;

	if (fmt == NULL) return;

	va_start(ap, fmt);
	vsprintf(text, fmt, ap);
	va_end(ap);

	glPushAttrib(GL_LIST_BIT);
	glListBase(base - 32);  // Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
	glPopAttrib();
}

void Spodek(void) {
	glPushMatrix();

	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	GLfloat radius = 60.0f;
	int segments = 64;

	glColor3f(0.49019f, 0.49411f, 0.458823f);

	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(0.0f, 0.0f, 0.0f);
	for (int i = 0; i <= segments; i++) {
		GLfloat angle = 2.0f * GL_PI * (float)i / (float)segments;

		GLfloat x = radius * cosf(angle);
		GLfloat z = radius * sinf(angle);

		glVertex3f(x, 0.0f, z);
	}
	glEnd();
}

void Spod(void) {
	glPushMatrix();

	GLfloat radius = 65.0f; 
	GLfloat height = 3.0f; 
	int segments = 64;      

	glColor3f(0.35f, 0.35f, 0.35f);

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= segments; i++) {
		GLfloat angle = 2.0f * GL_PI * (float)i / (float)segments;

		GLfloat x = radius * cosf(angle);
		GLfloat z = radius * sinf(angle);

		glVertex3f(x, 0.0f, z);
		glVertex3f(x, -height, z);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0f, -height, 0.0f);
	for (int i = 0; i <= segments; i++) {
		GLfloat angle = 2.0f * GL_PI * (float)i / (float)segments;

		GLfloat x = radius * cosf(angle);
		GLfloat z = radius * sinf(angle);

		glVertex3f(x, -height, z);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0f, 0.0f, 0.0f);
	for (int i = 0; i <= segments; i++) {
		GLfloat angle = 2.0f * GL_PI * (float)i / (float)segments;

		GLfloat x = radius * cosf(angle);
		GLfloat z = radius * sinf(angle);

		glVertex3f(x, 0.0f, z);
	}
	glEnd();

	glPopMatrix();
}

void SpodDek(void) {
	glPushMatrix();

	GLfloat radius = 65.1f;
	GLfloat height = 1.0f;
	int segments = 64;

	glColor3f(0.2941176470588235f, 1.0f, 0.4156862745098039f);

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= segments; i = i + 2) {
		GLfloat angle = 2.0f * GL_PI * (float)i / (float)segments;

		GLfloat x = radius * cosf(angle);
		GLfloat z = radius * sinf(angle);

		glVertex3f(x, 0.0f, z);
		glVertex3f(x, -height, z);
	}
	glEnd();

	glPopMatrix();
}

void SpodDek2(void) {
	glPushMatrix();

	GLfloat radius = 65.1f;
	GLfloat height = 1.0f;
	int segments = 64;

	glColor3f(0.6549019607843137f, 0.0588235294117647f, 1.0f);

	glBegin(GL_QUAD_STRIP);
	for (int i = 1; i <= segments; i = i + 2) {
		GLfloat angle = 2.0f * GL_PI * (float)i / (float)segments;

		GLfloat x = radius * cosf(angle);
		GLfloat z = radius * sinf(angle);

		glVertex3f(x, 0.0f, z);
		glVertex3f(x, -height, z);
	}
	glEnd();

	glPopMatrix();
}

void Kopula(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); {
		GLUquadricObj* obj = NULL;
		obj = gluNewQuadric();
		if (obj == NULL) return;

		glColor3f(0.5686f, 0.78222f, 0.9882f);

		gluQuadricTexture(obj, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glEnable(GL_TEXTURE_2D);

		// --- START LOGIC ---
		GLdouble planeEq[] = { 0.0, 0.0, 1.0, 0.0 };

		glClipPlane(GL_CLIP_PLANE0, planeEq);
		glEnable(GL_CLIP_PLANE0);

		gluSphere(obj, 20, 12, 7);

		glDisable(GL_CLIP_PLANE0);
		// --- END LOGIC ---

		glDisable(GL_TEXTURE_2D);

		gluDeleteQuadric(obj);
	}
}

void Rakieta(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); {
		GLUquadricObj* obj = NULL;
		obj = gluNewQuadric();
		if (obj == NULL) return;

		glColor3f(0.25f, 0.25f, 0.25f);

		gluQuadricTexture(obj, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glEnable(GL_TEXTURE_2D);

		// --- START LOGIC ---
		GLdouble planeEq[] = { 0.0, 0.0, 1.0, 0.0 };

		glClipPlane(GL_CLIP_PLANE0, planeEq);
		glEnable(GL_CLIP_PLANE0);

		gluSphere(obj, 24, 14, 9);

		glDisable(GL_CLIP_PLANE0);
		// --- END LOGIC ---

		glDisable(GL_TEXTURE_2D);

		gluDeleteQuadric(obj);
	}
}

void Swiatla(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); {
		GLUquadricObj* obj = NULL;
		obj = gluNewQuadric();
		if (obj == NULL) return;

		glColor3f(0.7803921568627451f, 0.7607843137254902f, 0.4901960784313725f);

		gluQuadricTexture(obj, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glEnable(GL_TEXTURE_2D);

		// --- START LOGIC ---
		GLdouble planeEq[] = { 0.0, 0.0, 1.0, 0.0 };

		glClipPlane(GL_CLIP_PLANE0, planeEq);
		glEnable(GL_CLIP_PLANE0);

		gluSphere(obj, 3, 12, 3);

		glDisable(GL_CLIP_PLANE0);
		// --- END LOGIC ---

		glDisable(GL_TEXTURE_2D);

		gluDeleteQuadric(obj);
	}
}

void UFO() {
	Spodek();

	glPushMatrix();
	glTranslatef(0.0f, -0.1f, 0.0f);
	Spod();
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(0.0f, -1.0f, 0.0f);
	glRotatef(decorationRotationAngle, 0.0f, 1.0f, 0.0f);
	SpodDek();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -1.0f, 0.0f);
	glRotatef(decorationRotationAngle, 0.0f, 1.0f, 0.0f);
	SpodDek2();
	glPopMatrix();
	
	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	Kopula();
	glPopMatrix();
	
	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0f, 0.0f, -15.0f);
	Rakieta();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0f, 55.0f, 0.0f);
	Swiatla();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0f, -55.0f, 0.0f);
	Swiatla();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(-55.0f, 0.0f, 0.0f);
	Swiatla();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(55.0f, 0.0f, 0.0f);
	Swiatla();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(38.9f, 38.9f, 0.0f);
	Swiatla();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(38.9f, -38.9f, 0.0f);
	Swiatla();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(-38.9f, 38.9f, 0.0f);
	Swiatla();
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(-38.9f, -38.9f, 0.0f);
	Swiatla();
	glPopMatrix();
}

void DrawGrid() {
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glColor3f(0.8f, 0.8f, 0.8f);

	glBegin(GL_LINES);
	for (float i = -500; i <= 500; i += 20) {
		glVertex3f(i, 0, -500);
		glVertex3f(i, 0, 500);

		glVertex3f(-500, 0, i);
		glVertex3f(500, 0, i);
	}
	glEnd();
	glPopMatrix();
}

void RenderScene(void)
{
	// Clear the window with current clearing color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // Reset the modelview matrix

	// --- Camera Setup  ---
	glTranslatef(0.0f, 0.0f, -cameraDistance);
	glTranslatef(cameraPanX, cameraPanY, 0.0f);

	glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f); // Pitch
	glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f); // Yaw

	glTranslatef(-ufoPos.x, -ufoPos.y, -ufoPos.z);
	// --- End Camera Setup ---

	// --- Teren
	DrawGrid();

	glPolygonMode(GL_BACK, GL_LINE);

	glPushMatrix();

	glTranslatef(ufoPos.x, ufoPos.y, ufoPos.z);
	glRotatef(steeringAngle, 0.0f, 0.0f, 1.0f);
	glRotatef(pitchAngle, 1.0f, 0.0f, 0.0f);

	UFO();
	glPopMatrix();

	// --- Text Rendering ---
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0, lastWidth, lastHeight, 0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glColor3f(0.0f, 0.0f, 0.0f);
	glRasterPos2i(10, 20);
	glPrint("TAB - odblokowanie myszki");

	glRasterPos2i(10, 35);
	glPrint("R - resetowanie kamery");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	//--- End Text Rendering ---

	glMatrixMode(GL_MODELVIEW);
	glFlush();
}


// Select the pixel format for a given device context
void SetDCPixelFormat(HDC hDC)
{
	int nPixelFormat;

	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),  // Size of this structure
		1,                                                              // Version of this structure    
		PFD_DRAW_TO_WINDOW |                    // Draw to Window (not to bitmap)
		PFD_SUPPORT_OPENGL |					// Support OpenGL calls in window
		PFD_DOUBLEBUFFER,                       // Double buffered
		PFD_TYPE_RGBA,                          // RGBA Color mode
		24,                                     // Want 24bit color 
		0,0,0,0,0,0,                            // Not used to select mode
		0,0,                                    // Not used to select mode
		0,0,0,0,0,                              // Not used to select mode
		32,                                     // Size of depth buffer
		0,                                      // Not used to select mode
		0,                                      // Not used to select mode
		PFD_MAIN_PLANE,                         // Draw in main plane
		0,                                      // Not used to select mode
		0,0,0 };                                // Not used to select mode

	// Choose a pixel format that best matches that described in pfd
	nPixelFormat = ChoosePixelFormat(hDC, &pfd);

	// Set the pixel format for the device context
	if (nPixelFormat != 0) {
		SetPixelFormat(hDC, nPixelFormat, &pfd);
	}
}


// If necessary, creates a 3-3-2 palette for the device context listed.
HPALETTE GetOpenGLPalette(HDC hDC)
{
	HPALETTE hRetPal = NULL;	// Handle to palette to be created
	PIXELFORMATDESCRIPTOR pfd;	// Pixel Format Descriptor
	LPLOGPALETTE pPal;			// Pointer to memory for logical palette
	int nPixelFormat;			// Pixel format index
	int nColors;				// Number of entries in palette
	int i;						// Counting variable
	BYTE RedRange, GreenRange, BlueRange;
	// Range for each color entry (7,7,and 3)


// Get the pixel format index and retrieve the pixel format description
	nPixelFormat = GetPixelFormat(hDC);
	DescribePixelFormat(hDC, nPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	// Does this pixel format require a palette?  If not, do not create a palette and just return NULL
	if (!(pfd.dwFlags & PFD_NEED_PALETTE))
		return NULL;

	// Number of entries in palette.  8 bits yeilds 256 entries
	nColors = 1 << pfd.cColorBits;

	// Allocate space for a logical palette structure plus all the palette entries
	pPal = (LPLOGPALETTE)malloc(sizeof(LOGPALETTE) + nColors * sizeof(PALETTEENTRY));

	if (pPal == NULL)
		return NULL;

	// Fill in palette header 
	pPal->palVersion = 0x300;		// Windows 3.0
	pPal->palNumEntries = (WORD)nColors; // table size

	RedRange = (BYTE)((1 << pfd.cRedBits) - 1);
	GreenRange = (BYTE)((1 << pfd.cGreenBits) - 1);
	BlueRange = (BYTE)((1 << pfd.cBlueBits) - 1);

	// Loop through all the palette entries
	for (i = 0; i < nColors; i++)
	{
		pPal->palPalEntry[i].peRed = (BYTE)((i >> pfd.cRedShift) & RedRange);
		pPal->palPalEntry[i].peRed = (BYTE)(
			((double)pPal->palPalEntry[i].peRed * 255.0) / RedRange);

		pPal->palPalEntry[i].peGreen = (BYTE)((i >> pfd.cGreenShift) & GreenRange);
		pPal->palPalEntry[i].peGreen = (BYTE)(
			((double)pPal->palPalEntry[i].peGreen * 255.0) / GreenRange);

		pPal->palPalEntry[i].peBlue = (BYTE)((i >> pfd.cBlueShift) & BlueRange);
		pPal->palPalEntry[i].peBlue = (BYTE)(
			((double)pPal->palPalEntry[i].peBlue * 255.0) / BlueRange);

		pPal->palPalEntry[i].peFlags = 0;
	}


	hRetPal = CreatePalette(pPal);

	SelectPalette(hDC, hRetPal, FALSE);
	RealizePalette(hDC);

	free(pPal);

	return hRetPal;
}


// Entry point of all Windows programs
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;			// Windows message structure
	WNDCLASS wc;		// Windows class structure
	HWND hWnd;          // Storeage for window handle

	hInstance = hInst;

	// Register Window style
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	// No need for background brush for OpenGL window
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	wc.lpszClassName = lpszAppName;

	// Register the window class
	if (RegisterClass(&wc) == 0)
		return FALSE;


	// Create the main application window
	hWnd = CreateWindow(
		lpszAppName,
		lpszAppName,

		// OpenGL requires WS_CLIPCHILDREN and WS_CLIPSIBLINGS
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,

		// Window position and size
		50, 50,
		1024, 768,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (hWnd == NULL)
		return FALSE;


	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}



// Window procedure, handles all messages for this program
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HGLRC hRC = NULL;
	static HDC hDC = NULL;

	switch (message)
	{
	case WM_CREATE:
		hDC = GetDC(hWnd);
		SetDCPixelFormat(hDC);
		hPalette = GetOpenGLPalette(hDC);

		// Create the rendering context and make it current
		hRC = wglCreateContext(hDC);
		if (!wglMakeCurrent(hDC, hRC)) {}

		SetupRC();
		BuildFont(hDC);
		glGenTextures(2, &texture[0]);

		if (isCursorLocked) ShowCursor(FALSE);

		// Load Textures
		bitmapData = LoadBitmapFile("checker.bmp", &bitmapInfoHeader);
		if (bitmapData != NULL) {
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, bitmapInfoHeader.biWidth,
				bitmapInfoHeader.biHeight, GL_RGB, GL_UNSIGNED_BYTE, bitmapData);
			free(bitmapData);
			bitmapData = NULL;
		}

		bitmapData = LoadBitmapFile("wheels.bmp", &bitmapInfoHeader);
		if (bitmapData != NULL) {
			glBindTexture(GL_TEXTURE_2D, texture[1]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, bitmapInfoHeader.biWidth,
				bitmapInfoHeader.biHeight, GL_RGB, GL_UNSIGNED_BYTE, bitmapData);
			free(bitmapData);
			bitmapData = NULL;
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		SetTimer(hWnd, 1, 10, NULL);
		break;

	case WM_DESTROY:
		KillFont();
		KillTimer(hWnd, 1);
		wglMakeCurrent(hDC, NULL);
		wglDeleteContext(hRC);
		ReleaseDC(hWnd, hDC);

		if (hPalette != NULL)
			DeleteObject(hPalette);

		PostQuitMessage(0);
		break;

	case WM_SIZE:
		ChangeSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_PAINT:
		RenderScene();
		SwapBuffers(hDC);
		ValidateRect(hWnd, NULL);
		break;

	case WM_QUERYNEWPALETTE:
		if (hPalette)
		{
			int nRet;
			SelectPalette(hDC, hPalette, FALSE);
			nRet = RealizePalette(hDC);
			InvalidateRect(hWnd, NULL, FALSE);
			return nRet;
		}
		break;

	case WM_PALETTECHANGED:
		if ((hPalette != NULL) && ((HWND)wParam != hWnd))
		{
			SelectPalette(hDC, hPalette, FALSE);
			RealizePalette(hDC);
			UpdateColors(hDC);
			return 0;
		}
		break;

	case WM_TIMER:
	{
		BOOL changed = FALSE;

		if (isDecorationsSpinning) {
			decorationRotationAngle -= 0.2f;
			if (decorationRotationAngle <= -360.0f) {
				decorationRotationAngle += 360.0f;
			}
			changed = TRUE;
		}

		if (keySpacja_held) ufoVelY += acceleration;
		else if (keyCTRL_held) ufoVelY -= acceleration;

		if (ufoPos.y < ufoMinHeight) {
			ufoPos.y = ufoMinHeight;
			ufoVelY = 0;
		}

		if (ufoPos.y < ufoMinHeight) {
			ufoPos.y = ufoMinHeight;
			ufoVelY = 0;
		}

		if (keyA_held) {
			ufoVelX -= acceleration;
			steeringAngle += steeringSpeed * 0.1f;
		}
		else if (keyD_held) {
			ufoVelX += acceleration;
			steeringAngle -= steeringSpeed * 0.1f;
		}
		
		if (keyW_held) {
			ufoVelZ -= acceleration;
			pitchAngle -= steeringSpeed * 0.1f;
		}
		else if (keyS_held) {
			ufoVelZ += acceleration;
			pitchAngle += steeringSpeed * 0.1f;
		}

		ufoVelX *= friction;
		ufoVelY *= friction;
		ufoVelZ *= friction;

		if (ufoVelX > ufoMaxSpeed)  ufoVelX = ufoMaxSpeed;
		if (ufoVelX < -ufoMaxSpeed) ufoVelX = -ufoMaxSpeed;
		if (ufoVelY > ufoMaxSpeed)  ufoVelY = ufoMaxSpeed;
		if (ufoVelY < -ufoMaxSpeed) ufoVelY = -ufoMaxSpeed;
		if (ufoVelZ > ufoMaxSpeed)  ufoVelZ = ufoMaxSpeed;
		if (ufoVelZ < -ufoMaxSpeed) ufoVelZ = -ufoMaxSpeed;

		float targetSteer = (ufoVelX / ufoMaxSpeed) * -maxSteeringAngle;
		float targetPitch = (ufoVelZ / ufoMaxSpeed) * maxPitchAngle;

		steeringAngle += (targetSteer - steeringAngle) * tiltSmoothing;
		pitchAngle += (targetPitch - pitchAngle) * tiltSmoothing;

		ufoPos.x += ufoVelX;
		ufoPos.y += ufoVelY;
		ufoPos.z += ufoVelZ;

		if (!keyA_held && !keyD_held) {
			steeringAngle *= 0.9f;
		}
		if (!keyW_held && !keyS_held) {
			pitchAngle *= 0.9f;
		}

		if (fabs(ufoVelX) > 0.01f || fabs(ufoVelY) > 0.01f || fabs(ufoVelZ) > 0.01f || changed) {
			changed = TRUE;
		}

		if (isResettingCamera) {
			float smoothSpeed = 0.1f;
			cameraAngleX += (0.0f - cameraAngleX) * smoothSpeed;
			cameraAngleY += (0.0f - cameraAngleY) * smoothSpeed;
			cameraPanX += (0.0f - cameraPanX) * smoothSpeed;
			cameraPanY += (0.0f - cameraPanY) * smoothSpeed;

			if (fabs(cameraAngleX) < 0.1f && fabs(cameraAngleY) < 0.1f && fabs(cameraPanX) < 0.1f && fabs(cameraPanY) < 0.1f) {
				cameraAngleX = 0.0f;
				cameraAngleY = 0.0f;
				cameraPanX = 0.0f;
				cameraPanY = 0.0f;
				isResettingCamera = FALSE;
			}
			changed = TRUE;
		}

		if (changed) {
			char debugBuffer[100];
			sprintf(debugBuffer, "X=%.2f	Y=%.2f	Z=%.2f \n", ufoPos.x, ufoPos.y, ufoPos.z);
			OutputDebugStringA(debugBuffer);
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}
	break;

	case WM_KEYDOWN:
	{
		BOOL changed = FALSE;

		if (wParam == VK_TAB) { 
			isCursorLocked = !isCursorLocked; 
			ShowCursor(!isCursorLocked); 
		
			if (isCursorLocked) isFirstMouse = TRUE;
		}

		if (wParam == VK_SPACE) { keySpacja_held = TRUE; keyCTRL_held = FALSE; changed = TRUE; }
		if (wParam == VK_CONTROL) { keyCTRL_held = TRUE; keySpacja_held = FALSE; changed = TRUE; }
		if (wParam == 'W' || wParam == 'w') { keyW_held = TRUE; keyS_held = FALSE; changed = TRUE; }
		if (wParam == 'S' || wParam == 's') { keyS_held = TRUE; keyW_held = FALSE; changed = TRUE; }
		if (wParam == 'A' || wParam == 'a') { keyA_held = TRUE; keyD_held = FALSE; changed = TRUE; }
		if (wParam == 'D' || wParam == 'd') { keyD_held = TRUE; keyA_held = FALSE; changed = TRUE; }
		if (wParam == 'R' || wParam == 'r') { isResettingCamera = TRUE; resetStartTime = GetTickCount64(); changed = TRUE; }
		if (changed) InvalidateRect(hWnd, NULL, FALSE);
	}
	break;

	case WM_KEYUP:
	{
		if (wParam == VK_SPACE) keySpacja_held = FALSE;
		if (wParam == VK_CONTROL) keyCTRL_held = FALSE;
		if (wParam == 'A' || wParam == 'a') keyA_held = FALSE;
		if (wParam == 'D' || wParam == 'd') keyD_held = FALSE;
		if (wParam == 'W' || wParam == 'w') keyW_held = FALSE;
		if (wParam == 'S' || wParam == 's') keyS_held = FALSE;
	}
	break;

	// --- Mouse Input Modified ---
	case WM_RBUTTONDOWN: // Start Panning
		isResettingCamera = FALSE;
		lastMouseX = LOWORD(lParam);
		lastMouseY = HIWORD(lParam);
		isPanning = TRUE;
		SetCapture(hWnd);
		break;

	case WM_RBUTTONUP: // Stop Panning
		isPanning = FALSE;
		ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
	{
		if (isCursorLocked)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			int centerX = (rect.right - rect.left) / 2;
			int centerY = (rect.bottom - rect.top) / 2;

			int x = LOWORD(lParam);
			int y = HIWORD(lParam);

			if (x == centerX && y == centerY)
				break;

			if (isFirstMouse) {
				POINT pt = { centerX, centerY };
				ClientToScreen(hWnd, &pt);
				SetCursorPos(pt.x, pt.y);
				isFirstMouse = FALSE;
				break;
			}

			int dx = x - centerX;
			int dy = y - centerY;

			if ((dx != 0 || dy != 0) && (GetTickCount64() - resetStartTime > 200)) {
				isResettingCamera = FALSE;
			}

			if (wParam & MK_RBUTTON)
			{
				cameraPanX += (GLfloat)dx * 0.2f;
				cameraPanY -= (GLfloat)dy * 0.2f;
			}
			else
			{
				cameraAngleY += (GLfloat)dx * 0.2f; // Yaw
				cameraAngleX += (GLfloat)dy * 0.2f; // Pitch

				if (cameraAngleY > 180.0f) cameraAngleY -= 360.0f;
				if (cameraAngleY < -180.0f) cameraAngleY += 360.0f;

				if (cameraAngleX > 89.0f) cameraAngleX = 89.0f;
				if (cameraAngleX < -89.0f) cameraAngleX = -89.0f;
			}

			POINT pt = { centerX, centerY };
			ClientToScreen(hWnd, &pt);
			SetCursorPos(pt.x, pt.y);

			InvalidateRect(hWnd, NULL, FALSE);
		}
		else
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);

			if ((x != lastMouseX || y != lastMouseY) && (GetTickCount64() - resetStartTime > 200)) {
				isResettingCamera = FALSE;
			}

			lastMouseX = x;
			lastMouseY = y;
		}
	}
	break;

	case WM_MOUSEWHEEL: // Zoom
	{
		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		cameraDistance -= (float)zDelta / 10.0f;
		if (cameraDistance < 20.0f) cameraDistance = 20.0f;
		if (cameraDistance > 500.0f) cameraDistance = 500.0f;
		InvalidateRect(hWnd, NULL, FALSE);
	}
	break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_FILE_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_HELP_ABOUT:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hWnd, (DLGPROC)AboutDlgProc);
			break;
		}
	}
	break;

	default:
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0L);
}


// Dialog procedure.
BOOL APIENTRY AboutDlgProc(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{

	switch (message)
	{
		// Initialize the dialog box
	case WM_INITDIALOG:
	{
		int i;
		GLenum glError;

		// glGetString demo
		SetDlgItemText(hDlg, IDC_OPENGL_VENDOR, (LPCSTR)glGetString(GL_VENDOR));
		SetDlgItemText(hDlg, IDC_OPENGL_RENDERER, (LPCSTR)glGetString(GL_RENDERER));
		SetDlgItemText(hDlg, IDC_OPENGL_VERSION, (LPCSTR)glGetString(GL_VERSION));
		SetDlgItemText(hDlg, IDC_OPENGL_EXTENSIONS, (LPCSTR)glGetString(GL_EXTENSIONS));

		// gluGetString demo
		SetDlgItemText(hDlg, IDC_GLU_VERSION, (LPCSTR)gluGetString(GLU_VERSION));
		SetDlgItemText(hDlg, IDC_GLU_EXTENSIONS, (LPCSTR)gluGetString(GLU_EXTENSIONS));


		// Display any recent error messages
		i = 0;
		do {
			glError = glGetError();
			if (glError != GL_NO_ERROR) {
				SetDlgItemText(hDlg, IDC_ERROR1 + i, (LPCSTR)gluErrorString(glError));
			}
			else {
				SetDlgItemText(hDlg, IDC_ERROR1 + i, TEXT("No Error"));
			}
			i++;
		} while (i < 6 && glError != GL_NO_ERROR);

		while (i < 6) {
			SetDlgItemText(hDlg, IDC_ERROR1 + i, TEXT(""));
			i++;
		}

		return (TRUE);
	}
	break;

	// Process command messages
	case WM_COMMAND:
	{
		// Validate and Make the changes
		if (LOWORD(wParam) == IDOK)
			EndDialog(hDlg, TRUE);
	}
	break;

	// Closed from sysbox
	case WM_CLOSE:
		EndDialog(hDlg, TRUE);
		break;
	}

	return FALSE;
}