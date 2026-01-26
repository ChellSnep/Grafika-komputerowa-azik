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

// Teren
GLfloat GetTerrainHeight(GLfloat x, GLfloat z) {
	return 3.0f * sinf(x * 0.08f) * cosf(z * 0.08f) +
		2.0f * sinf(x * 0.15f + z * 0.15f);
}

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
	FILE* filePtr = NULL;
	BITMAPFILEHEADER bitmapFileHeader;

	filePtr = fopen(filename, "rb");
	if (!filePtr) {
		return NULL;
	}

	// Read file header
	if (fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr) != 1) {
		fclose(filePtr);
		return NULL;
	}

	// Check for 'BM'
	if (bitmapFileHeader.bfType != 0x4D42) {
		fclose(filePtr);
		return NULL;
	}

	// Read info header
	if (fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr) != 1) {
		fclose(filePtr);
		return NULL;
	}

	int width = bitmapInfoHeader->biWidth;
	int height = abs(bitmapInfoHeader->biHeight);
	int bpp = bitmapInfoHeader->biBitCount;

	if (width <= 0 || height <= 0 || (bpp != 24 && bpp != 32)) {
		fclose(filePtr);
		return NULL;
	}

	// Move to pixel array
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	size_t bytesPerPixel = bpp / 8;
	size_t rowPadded = ((width * bytesPerPixel + 3) & ~3);
	size_t dataSize = rowPadded * height;

	unsigned char* rawData = (unsigned char*)malloc(dataSize);
	if (!rawData) {
		fclose(filePtr);
		return NULL;
	}

	// Read pixel data (BGR or BGRA)
	if (fread(rawData, 1, dataSize, filePtr) != dataSize) {
		free(rawData);
		fclose(filePtr);
		return NULL;
	}

	fclose(filePtr);

	// OpenGL requires textures stored bottom-up → BMP stores bottom-up
	// NOTHING needs to be flipped.

	// We return raw BGR data; OpenGL will use GL_BGR_EXT
	bitmapInfoHeader->biSizeImage = dataSize;

	return rawData;
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

	glPopMatrix();
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

void DrawTerrain() {
	glPushMatrix();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	// Disable lighting if it interferes
	glDisable(GL_LIGHTING);

	const GLfloat step = 20.0f;
	const GLfloat tileSize = 40.0f;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	// IMPORTANT: Set to REPLACE mode to see texture clearly
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// No color tinting - let texture show through
	glColor3f(1.0f, 1.0f, 1.0f);

	for (float x = -500; x < 500; x += step) {
		for (float z = -500; z < 500; z += step) {

			GLfloat h1 = GetTerrainHeight(x, z);
			GLfloat h2 = GetTerrainHeight(x + step, z);
			GLfloat h3 = GetTerrainHeight(x + step, z + step);
			GLfloat h4 = GetTerrainHeight(x, z + step);

			GLfloat u1 = x / tileSize;
			GLfloat v1 = z / tileSize;
			GLfloat u2 = (x + step) / tileSize;
			GLfloat v2 = (z + step) / tileSize;

			glBegin(GL_QUADS);
			glTexCoord2f(u1, v1); glVertex3f(x, h1, z);
			glTexCoord2f(u2, v1); glVertex3f(x + step, h2, z);
			glTexCoord2f(u2, v2); glVertex3f(x + step, h3, z + step);
			glTexCoord2f(u1, v2); glVertex3f(x, h4, z + step);
			glEnd();
		}
	}

	// Reset to MODULATE for other objects
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

void RenderScene(void)
{
	// Clear the window with current clearing color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// --- Camera Setup  ---
	glTranslatef(0.0f, 0.0f, -cameraDistance);
	glTranslatef(cameraPanX, cameraPanY, 0.0f);

	glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f); // Pitch
	glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f); // Yaw

	glTranslatef(-ufoPos.x, -ufoPos.y, -ufoPos.z);
	// --- End Camera Setup ---

	// --- Draw Terrain
	DrawTerrain();

	// --- Draw UFO
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // Ensure fill mode
	glEnable(GL_DEPTH_TEST);

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

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	//--- End Text Rendering ---

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
	// DIAGNOSTYKA: pokaż ścieżkę, fopen, i nagłówki BMP
	char exePath[MAX_PATH];
	char exeDir[MAX_PATH];
	char resourcePath[MAX_PATH];

	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	strcpy_s(exeDir, MAX_PATH, exePath);
	char* lastSlash = strrchr(exeDir, '\\');
	if (lastSlash) *lastSlash = '\0';
	_snprintf_s(resourcePath, MAX_PATH, _TRUNCATE, "%s\\Resources\\water.bmp", exeDir);

	{
		char msg[MAX_PATH * 2];
		sprintf_s(msg, sizeof(msg), "Trying to load (full path):\n%s\n\nWorking directory:\n", resourcePath);
		char cwd[MAX_PATH]; GetCurrentDirectoryA(MAX_PATH, cwd);
		strcat_s(msg, sizeof(msg), cwd);
		MessageBoxA(NULL, msg, "Diag: paths", MB_OK);
	}

	// fopen_s test
	FILE* f = NULL;
	errno_t ferr = fopen_s(&f, resourcePath, "rb");
	if (ferr != 0 || f == NULL) {
		char msg[200];
		sprintf_s(msg, sizeof(msg), "fopen_s FAILED (errno = %d). File cannot be opened by this process.", ferr);
		MessageBoxA(NULL, msg, "Diag: fopen", MB_OK | MB_ICONERROR);
	}
	else {
		// if file opened, read headers
		BITMAPFILEHEADER bf;
		BITMAPINFOHEADER bi;
		size_t nread = fread(&bf, 1, sizeof(bf), f);
		nread += fread(&bi, 1, sizeof(bi), f);
		// rewind just in case
		fseek(f, 0, SEEK_SET);

		char hdrmsg[400];
		sprintf_s(hdrmsg, sizeof(hdrmsg),
			"fopen_s SUCCEEDED.\n\nbfType = 0x%04X\nbfOffBits = %u\n\nbiWidth = %d\nbiHeight = %d\nbiBitCount = %u\nbiSizeImage = %u\nbiCompression = %u",
			bf.bfType, bf.bfOffBits, bi.biWidth, bi.biHeight, bi.biBitCount, bi.biSizeImage, bi.biCompression);

		MessageBoxA(NULL, hdrmsg, "Diag: BMP header", MB_OK);

		fclose(f);
	}

	// Teraz spróbuj załadować normalnie i pokaż czy LoadBitmapFile zwróci NULL
	unsigned char* testBitmap = LoadBitmapFile(resourcePath, &bitmapInfoHeader);
	if (testBitmap == NULL) {
		MessageBoxA(NULL, "LoadBitmapFile returned NULL (wczytywanie nie powiodło się).", "Diag: loader", MB_OK | MB_ICONERROR);
	}
	else {
		MessageBoxA(NULL, "LoadBitmapFile SUCCEEDED.", "Diag: loader", MB_OK);
		free(testBitmap);
	}

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
	{
		hDC = GetDC(hWnd);
		SetDCPixelFormat(hDC);
		hPalette = GetOpenGLPalette(hDC);

		hRC = wglCreateContext(hDC);
		wglMakeCurrent(hDC, hRC);

		SetupRC();
		BuildFont(hDC);

		// Generate TWO textures
		glGenTextures(2, texture);

		// Load water.bmp for terrain (texture[0])
		bitmapData = LoadBitmapFile("..\\Resources\\water.bmp", &bitmapInfoHeader);
		if (bitmapData)
		{
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			// <- WAŻNE: wyrównanie
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if (bitmapInfoHeader.biBitCount == 24) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
					(GLsizei)bitmapInfoHeader.biWidth, (GLsizei)abs(bitmapInfoHeader.biHeight),
					0, GL_BGR_EXT, GL_UNSIGNED_BYTE, bitmapData);
			}
			else if (bitmapInfoHeader.biBitCount == 32) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					(GLsizei)bitmapInfoHeader.biWidth, (GLsizei)abs(bitmapInfoHeader.biHeight),
					0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bitmapData);
			}

			free(bitmapData);
			bitmapData = NULL;

			MessageBoxA(NULL, "water.bmp loaded successfully!", "Success", MB_OK);
		}
		else
		{
			MessageBoxA(NULL, "water.bmp NOT FOUND! Using fallback texture.", "Warning", MB_OK);
			// Create fallback blue checkerboard texture
			const int size = 64;
			unsigned char* checkerboard = (unsigned char*)malloc(size * size * 3);
			for (int i = 0; i < size; i++) {
				for (int j = 0; j < size; j++) {
					// Check if this square is dark or light
					int isDark = ((i & 0x8) == 0) ^ ((j & 0x8) == 0);

					if (isDark) {
						// Dark blue: RGB(30, 80, 150)
						checkerboard[(i * size + j) * 3 + 0] = 30;   // Red
						checkerboard[(i * size + j) * 3 + 1] = 80;   // Green
						checkerboard[(i * size + j) * 3 + 2] = 150;  // Blue
					}
					else {
						// Light blue: RGB(100, 180, 255)
						checkerboard[(i * size + j) * 3 + 0] = 100;  // Red
						checkerboard[(i * size + j) * 3 + 1] = 180;  // Green
						checkerboard[(i * size + j) * 3 + 2] = 255;  // Blue
					}
				}
			}
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if (bitmapInfoHeader.biBitCount == 24) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
					(GLsizei)bitmapInfoHeader.biWidth, (GLsizei)abs(bitmapInfoHeader.biHeight),
					0, GL_BGR_EXT, GL_UNSIGNED_BYTE, bitmapData);
			}
			else if (bitmapInfoHeader.biBitCount == 32) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					(GLsizei)bitmapInfoHeader.biWidth, (GLsizei)abs(bitmapInfoHeader.biHeight),
					0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bitmapData);
			}
			free(checkerboard);
		}

		// Load chrome.bmp for UFO (texture[1]) - or create a simple default texture
		bitmapData = LoadBitmapFile("..\\Resources\\chrome.bmp", &bitmapInfoHeader);
		if (bitmapData)
		{
			glBindTexture(GL_TEXTURE_2D, texture[1]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if (bitmapInfoHeader.biBitCount == 24) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
					(GLsizei)bitmapInfoHeader.biWidth, (GLsizei)abs(bitmapInfoHeader.biHeight),
					0, GL_BGR_EXT, GL_UNSIGNED_BYTE, bitmapData);
			}
			else if (bitmapInfoHeader.biBitCount == 32) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					(GLsizei)bitmapInfoHeader.biWidth, (GLsizei)abs(bitmapInfoHeader.biHeight),
					0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bitmapData);
			}

			free(bitmapData);
			bitmapData = NULL;
		}
		else
		{
			// Create a simple white texture as fallback
			glBindTexture(GL_TEXTURE_2D, texture[1]);
			unsigned char whiteTexture[3] = { 255, 255, 255 };
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whiteTexture);
		}

		// --- Start: debug i bezpieczne ładowanie tekstury water.bmp ---
		char exePath[MAX_PATH];
		char exeDir[MAX_PATH];
		char resourcePath[MAX_PATH];

		// Pobierz pełną ścieżkę do uruchamianego pliku .exe
		GetModuleFileNameA(NULL, exePath, MAX_PATH);

		// Wytnij nazwę pliku .exe, zostaje katalog
		strcpy_s(exeDir, MAX_PATH, exePath);
		char* lastSlash = strrchr(exeDir, '\\');
		if (lastSlash) *lastSlash = '\0';

		// Złóż ścieżkę do Resources\water.bmp obok exe
		_snprintf_s(resourcePath, MAX_PATH, _TRUNCATE, "%s\\Resources\\water.bmp", exeDir);

		// Pokaż ścieżkę w okienku — ułatwia debug
		MessageBoxA(NULL, resourcePath, "Trying to load (full path)", MB_OK);

		// Dodatkowy test: czy plik w ogóle istnieje (fopen)
		FILE* ftest = fopen(resourcePath, "rb");
		if (ftest) {
			fclose(ftest);
		}
		else {
			// Jeśli nie istnieje, pokaż też bieżący katalog roboczy procesu
			char cwd[MAX_PATH];
			GetCurrentDirectoryA(MAX_PATH, cwd);
			char msg[MAX_PATH * 2];
			_snprintf_s(msg, sizeof(msg), _TRUNCATE,
				"File not found at:\n%s\n\nCurrent working directory:\n%s",
				resourcePath, cwd);
			MessageBoxA(NULL, msg, "File NOT FOUND", MB_OK | MB_ICONWARNING);
		}

		// Teraz używamy resourcePath do załadowania bitmapy
		bitmapData = LoadBitmapFile(resourcePath, &bitmapInfoHeader);
		if (bitmapData) {
			// ... (twój istniejący kod inicjujący teksturę)
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
				(GLsizei)bitmapInfoHeader.biWidth, (GLsizei)abs(bitmapInfoHeader.biHeight),
				0, GL_RGB, GL_UNSIGNED_BYTE, bitmapData);

			free(bitmapData);
			bitmapData = NULL;
			MessageBoxA(NULL, "water.bmp loaded successfully!", "Success", MB_OK);
		}
		else {
			MessageBoxA(NULL, "water.bmp NOT FOUND at resourcePath! Using fallback texture.", "Warning", MB_OK);
			// ... (twój fallback checkerboard)
		}
		// --- End: debug i bezpieczne ładowanie tekstury water.bmp ---


		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		SetTimer(hWnd, 1, 10, NULL);
		break;
	}

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