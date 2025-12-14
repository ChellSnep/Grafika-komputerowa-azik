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

// --- New Camera Variables for Orbiting/Zooming/Panning ---
static GLfloat cameraDistance = 200.0f;
static GLfloat cameraAngleX = 0.0f; 
static GLfloat cameraAngleY = 0.0f;

// Pan/Translate variables
static GLfloat cameraPanX = 0.0f;
static GLfloat cameraPanY = 0.0f;

// Mouse state for interaction
static int lastMouseX;
static int lastMouseY;
static BOOL isRotating = FALSE;
static BOOL isPanning = FALSE;
// --- End New Camera Variables ---

// --- New Movement Variables ---
static GLfloat wheelRotationAngle = 0.0f;
static GLfloat steeringAngle = 0.0f;
static const GLfloat maxSteeringAngle = 25.0f;
static const GLfloat steeringSpeed = 5.0f;
// --- End New Movement Variables ---

static BOOL keyA_held = FALSE;
static BOOL keyD_held = FALSE;

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
	gluPerspective(60.0f, fAspect, 1.0, 5000.0); // Use perspective

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


// This function does any needed initialization on the rendering
// context.  Here it sets up and initializes the lighting for
// the scene.
void SetupRC()
{
	// Light values and coordinates
	//GLfloat  ambientLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	//GLfloat  diffuseLight[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	//GLfloat  specular[] = { 1.0f, 1.0f, 1.0f, 1.0f};
	//GLfloat	 lightPos[] = { 0.0f, 150.0f, 150.0f, 1.0f };
	//GLfloat  specref[] =  { 1.0f, 1.0f, 1.0f, 1.0f };


	glEnable(GL_DEPTH_TEST);	// Hidden surface removal
	glFrontFace(GL_CCW);		// Counter clock-wise polygons face out
	//glEnable(GL_CULL_FACE);		// Do not calculate inside of jet

	// Enable lighting
	//glEnable(GL_LIGHTING);

	// Setup and enable light 0
	//glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
	//glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	//glLightfv(GL_LIGHT0,GL_SPECULAR,specular);
	//glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
	//glEnable(GL_LIGHT0);

	// Enable color tracking
	//glEnable(GL_COLOR_MATERIAL);

	// Set Material properties to follow glColor values
	//glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// All materials hereafter have full specular reflectivity
	// with a high shine
	//glMaterialfv(GL_FRONT, GL_SPECULAR,specref);
	//glMateriali(GL_FRONT,GL_SHININESS,128);


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

void kabina(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	{
		GLfloat x2 = 24.5f;
		GLfloat y2 = 25.0f;
		GLfloat z2 = 25.0f;

		glColor3f(1.0f, 0.0f, 0.0f);

		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x2, -y2, z2);
		glVertex3f(x2, -y2, z2);
		glVertex3f(-x2, y2, z2);
		glVertex3f(x2, y2, z2);
		glEnd();

		// --- TYŁ (Back) ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x2, -y2, -z2);
		glVertex3f(x2, -y2, -z2);
		glVertex3f(-x2, y2, -z2);
		glVertex3f(x2, y2, -z2);
		glEnd();

		// --- LEWA (Left) ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x2, -y2, -z2);
		glVertex3f(-x2, -y2, z2);
		glVertex3f(-x2, y2, -z2);
		glVertex3f(-x2, y2, z2);
		glEnd();

		// --- PRAWA (Right) ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(x2, -y2, -z2);
		glVertex3f(x2, -y2, z2);
		glVertex3f(x2, y2, -z2);
		glVertex3f(x2, y2, z2);
		glEnd();

		// --- DÓŁ (Bottom) ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x2, -y2, -z2);
		glVertex3f(x2, -y2, -z2);
		glVertex3f(-x2, -y2, z2);
		glVertex3f(x2, -y2, z2);
		glEnd();

		// --- GÓRA (Top) ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x2, y2, -z2);
		glVertex3f(x2, y2, -z2);
		glVertex3f(-x2, y2, z2);
		glVertex3f(x2, y2, z2);
		glEnd();
	}
}

void body(void)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	{
		GLfloat x1 = 25.0f;
		GLfloat y1 = 70.0f;
		GLfloat z1 = 15.0f;

		glColor3f(0.0f, 1.0f, 0.0f);


		// --- PRZ�D ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x1, -y1, z1);
		glVertex3f(x1, -y1, z1);
		glVertex3f(-x1, y1, z1);
		glVertex3f(x1, y1, z1);
		glEnd();

		// --- TY� ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x1, -y1, -z1);
		glVertex3f(x1, -y1, -z1);
		glVertex3f(-x1, y1, -z1);
		glVertex3f(x1, y1, -z1);
		glEnd();

		// --- LEWA ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x1, -y1, -z1);
		glVertex3f(-x1, -y1, z1);
		glVertex3f(-x1, y1, -z1);
		glVertex3f(-x1, y1, z1);
		glEnd();

		// --- PRAWA ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(x1, -y1, -z1);
		glVertex3f(x1, -y1, z1);
		glVertex3f(x1, y1, -z1);
		glVertex3f(x1, y1, z1);
		glEnd();

		// --- Dӣ ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x1, -y1, -z1);
		glVertex3f(x1, -y1, -z1);
		glVertex3f(-x1, -y1, z1);
		glVertex3f(x1, -y1, z1);
		glEnd();

		// --- G�RA ---
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-x1, y1, -z1);
		glVertex3f(x1, y1, -z1);
		glVertex3f(-x1, y1, z1);
		glVertex3f(x1, y1, z1);
		glEnd();
	}

}

void kula(void)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); {
		GLUquadricObj* obj = NULL;
		obj = gluNewQuadric();
		if (obj == NULL) return;

		glColor3f(0.1f, 0.1f, 0.1f);

		gluQuadricTexture(obj, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glEnable(GL_TEXTURE_2D);
		gluSphere(obj, 15, 10, 7);
		glDisable(GL_TEXTURE_2D);

		gluDeleteQuadric(obj);
	}
}

void kulaDuza(void)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); {
		GLUquadricObj* obj = NULL;
		obj = gluNewQuadric();
		if (obj == NULL) return;

		glColor3f(0.1f, 0.1f, 0.1f);

		gluQuadricTexture(obj, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glEnable(GL_TEXTURE_2D);
		gluSphere(obj, 20, 12, 7);
		glDisable(GL_TEXTURE_2D);

		gluDeleteQuadric(obj);
	}
}


void Maszyna() {
	// --- Głównie ciało + kabina --- 
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.0f);
	glTranslatef(25.0f, 37.5f, -7.0f);
	body();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.0f);
	glTranslatef(25.0f, 82.5f, -37.0f);
	kabina();
	glPopMatrix();
	// --- Głównie ciało + kabina --- 

	// --- Przednie koła --
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.0f);
	glRotatef(wheelRotationAngle, 1.0f, 0.0f, 0.0f);
	glRotatef(steeringAngle, 0.0f, 0.0f, 1.0f);
	kula();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(50.0f, 0.0f, 0.0f);
	glRotatef(wheelRotationAngle, 1.0f, 0.0f, 0.0f);
	glRotatef(steeringAngle, 0.0f, 0.0f, 1.0f);
	kula();
	glPopMatrix();
	// --- Przednie koła --

	// --- Tylnie koła ---
	glPushMatrix();
	glTranslatef(0.0f, 75.0f, -5.0f);
	glRotatef(wheelRotationAngle, 1.0f, 0.0f, 0.0f);
	kulaDuza();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(50.0f, 75.0f, -5.0f);
	glRotatef(wheelRotationAngle, 1.0f, 0.0f, 0.0f);
	kulaDuza();
	glPopMatrix();
	// --- Tylnie koła ---
}

void RenderScene(void)
{
	// Clear the window with current clearing color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // Reset the modelview matrix

	// --- New Camera Setup (View Transformation) ---

	// 1. Zoom (Move the camera back/forward along the view vector)
	glTranslatef(0.0f, 0.0f, -cameraDistance);

	// 2. Pan (Translate the world in the opposite direction of the pan)
	glTranslatef(cameraPanX, cameraPanY, 0.0f);

	// 3. Orbit (Rotate the world around the center (0,0,0) of Maszyna)
	glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f); // Pitch
	glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f); // Yaw
	// --- End New Camera Setup ---

	glPolygonMode(GL_BACK, GL_LINE);

	Maszyna();

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

	// Does this pixel format require a palette?  If not, do not create a
	// palette and just return NULL
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

	// Build mask of all 1's.  This creates a number represented by having the low order x bits set, where x = pfd.cRedBits, pfd.cGreenBits, and pfd.cBlueBits.
	RedRange = (BYTE)((1 << pfd.cRedBits) - 1);
	GreenRange = (BYTE)((1 << pfd.cGreenBits) - 1);
	BlueRange = (BYTE)((1 << pfd.cBlueBits) - 1);

	// Loop through all the palette entries
	for (i = 0; i < nColors; i++)
	{
		// Fill in the 8-bit equivalents for each component
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


	// Create the palette
	hRetPal = CreatePalette(pPal);

	// Go ahead and select and realize the palette for this device context
	SelectPalette(hDC, hRetPal, FALSE);
	RealizePalette(hDC);

	// Free the memory used for the logical palette structure
	free(pPal);

	// Return the handle to the new palette
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
		400, 400,
		NULL,
		NULL,
		hInstance,
		NULL);

	// If window was not created, quit
	if (hWnd == NULL)
		return FALSE;


	// Display the window
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Process application messages until the application closes
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}




// Window procedure, handles all messages for this program
LRESULT CALLBACK WndProc(HWND    hWnd,
	UINT    message,
	WPARAM  wParam,
	LPARAM  lParam)
{
	static HGLRC hRC = NULL;               // Permenant Rendering context
	static HDC hDC = NULL;                 // Private GDI Device context

	switch (message)
	{
		// Window creation, setup for OpenGL
	case WM_CREATE:
		// Store the device context
		hDC = GetDC(hWnd);

		// Select the pixel format
		SetDCPixelFormat(hDC);

		// Create palette if needed
		hPalette = GetOpenGLPalette(hDC);

		// Create the rendering context and make it current
		hRC = wglCreateContext(hDC);
		if (!wglMakeCurrent(hDC, hRC)) {
			// Handle error
		}

		SetupRC();
		glGenTextures(2, &texture[0]);                 // tworzy obiekt tekstury			

		// �aduje pierwszy obraz tekstury:
		bitmapData = LoadBitmapFile("checker.bmp", &bitmapInfoHeader);

		if (bitmapData != NULL) {
			glBindTexture(GL_TEXTURE_2D, texture[0]);       // aktywuje obiekt tekstury

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, bitmapInfoHeader.biWidth,
				bitmapInfoHeader.biHeight, GL_RGB, GL_UNSIGNED_BYTE, bitmapData);

			free(bitmapData);
			bitmapData = NULL;
		}
				
		if (bitmapData != NULL) {
			glBindTexture(GL_TEXTURE_2D, texture[1]);   // activate texture object

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			// create texture image
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, bitmapInfoHeader.biWidth,
				bitmapInfoHeader.biHeight, GL_RGB, GL_UNSIGNED_BYTE, bitmapData);

			free(bitmapData);
			bitmapData = NULL;
		}

		// ustalenie sposobu mieszania tekstury z t�em
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		SetTimer(hWnd, 1, 10, NULL);
		break;

		// Window is being destroyed, cleanup
	case WM_DESTROY:
		KillTimer(hWnd, 1);
		// Deselect the current rendering context and delete it
		wglMakeCurrent(hDC, NULL);
		wglDeleteContext(hRC);

		ReleaseDC(hWnd, hDC);

		// Delete the palette if it was created
		if (hPalette != NULL)
			DeleteObject(hPalette);

		PostQuitMessage(0);
		break;

	case WM_SIZE:
		// Call our function which modifies the clipping volume and viewport
		ChangeSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_PAINT:
	{
		// Call OpenGL drawing code
		RenderScene();

		SwapBuffers(hDC);

		// Validate the newly painted client area
		ValidateRect(hWnd, NULL);
	}
	break;

	// Windows is telling the application that it may modify the system palette.  This message in essance asks the application for a new palette.
	case WM_QUERYNEWPALETTE:
		// If the palette was created.
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
		// Don't do anything if the palette does not exist, or if this is the window that changed the palette.
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

		if (keyA_held) {
			steeringAngle -= steeringSpeed * 0.1f;
			if (steeringAngle < -maxSteeringAngle) steeringAngle = -maxSteeringAngle;
			changed = TRUE;
		}
		else if (keyD_held) {
			steeringAngle += steeringSpeed * 0.1f;
			if (steeringAngle > maxSteeringAngle) steeringAngle = maxSteeringAngle;
			changed = TRUE;
		}
		else {
			if (fabs(steeringAngle) > 0.5f) {
				if (steeringAngle > 0) {
					steeringAngle -= steeringSpeed * 0.15f;
					if (steeringAngle < 0.0f) steeringAngle = 0.0f;
				}
				else {
					steeringAngle += steeringSpeed * 0.15f;
					if (steeringAngle > 0.0f) steeringAngle = 0.0f;
				}
				changed = TRUE;
			}
			else if (steeringAngle != 0.0f) {
				steeringAngle = 0.0f;
				changed = TRUE;
			}
		}

		if (changed) {
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}
	break;

	// Key press, check for arrow keys to do cube rotation.
	case WM_KEYDOWN:
	{
		BOOL changed = FALSE;
		GLfloat rotationSpeed = 10.0f;

		// W: Rotate Wheels Forward (Simulated Drive Forward)
		if (wParam == 'W' || wParam == 'w') {
			wheelRotationAngle -= rotationSpeed; // Decrement for forward look (rotation around Z)
			wheelRotationAngle = fmodf(wheelRotationAngle, 360.0f);
			changed = TRUE;
		}

		// S: Rotate Wheels Backward (Simulated Drive Backward)
		if (wParam == 'S' || wParam == 's') {
			wheelRotationAngle += rotationSpeed; // Increment for backward look (rotation around Z)
			wheelRotationAngle = fmodf(wheelRotationAngle, 360.0f);
			changed = TRUE;
		}

		// A: Turn Left (Steering)
		if (wParam == 'A' || wParam == 'a') {
			keyA_held = TRUE;
			keyD_held = FALSE; // Ensure only one is pressed
			changed = TRUE;
		}

		// D: Turn Right (Steering)
		if (wParam == 'D' || wParam == 'd') {
			keyD_held = TRUE;
			keyA_held = FALSE; // Ensure only one is pressed
			changed = TRUE;
		}

		// Arrow Keys: Pan (alternative to mouse pan)
		if (wParam == VK_LEFT) {
			cameraPanX += 5.0f;
			changed = TRUE;
		}

		if (wParam == VK_RIGHT) {
			cameraPanX -= 5.0f;
			changed = TRUE;
		}

		if (wParam == VK_UP) {
			cameraPanY -= 5.0f; // Y-axis is up in screen space, up key should pan "up" in world
			changed = TRUE;
		}

		if (wParam == VK_DOWN) {
			cameraPanY += 5.0f;
			changed = TRUE;
		}

		if (changed) {
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}
	break;

	case WM_KEYUP:
	{
		if (wParam == 'A' || wParam == 'a') {
			keyA_held = FALSE;
		}
		if (wParam == 'D' || wParam == 'd') {
			keyD_held = FALSE;
		}
	}
	break;

	// Mouse input for Orbit (LMB) and Pan (RMB)
	case WM_LBUTTONDOWN: // Start Orbiting
		lastMouseX = LOWORD(lParam);
		lastMouseY = HIWORD(lParam);
		isRotating = TRUE;
		SetCapture(hWnd); // Capture mouse input
		break;

	case WM_RBUTTONDOWN: // Start Panning
		lastMouseX = LOWORD(lParam);
		lastMouseY = HIWORD(lParam);
		isPanning = TRUE;
		SetCapture(hWnd); // Capture mouse input
		break;

	case WM_MOUSEMOVE:
		if (wParam & MK_LBUTTON) // Left button down (Orbiting)
		{
			if (isRotating) {
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);

				// Calculate change in position
				int dx = x - lastMouseX;
				int dy = y - lastMouseY;

				// Update camera angles (a factor of 0.5 is a good starting sensitivity)
				cameraAngleY += (GLfloat)dx * 0.5f; // Yaw (around Y-axis)
				cameraAngleX += (GLfloat)dy * 0.5f; // Pitch (around X-axis)

				// Keep angles within 0-360 range
				cameraAngleY = fmodf(cameraAngleY, 360.0f);
				cameraAngleX = fmodf(cameraAngleX, 360.0f);

				lastMouseX = x;
				lastMouseY = y;

				InvalidateRect(hWnd, NULL, FALSE);
			}
		}
		else if (wParam & MK_RBUTTON) // Right button down (Panning)
		{
			if (isPanning) {
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);

				// Calculate change in position
				int dx = x - lastMouseX;
				int dy = y - lastMouseY;

				// Pan/Translate (a factor of 0.2 is a good starting sensitivity)
				cameraPanX += (GLfloat)dx * 0.2f;
				cameraPanY -= (GLfloat)dy * 0.2f; // Invert Y because screen Y increases downwards

				lastMouseX = x;
				lastMouseY = y;

				InvalidateRect(hWnd, NULL, FALSE);
			}
		}
		break;

	case WM_LBUTTONUP: // Stop Orbiting
		isRotating = FALSE;
		ReleaseCapture();
		break;

	case WM_RBUTTONUP: // Stop Panning
		isPanning = FALSE;
		ReleaseCapture();
		break;

	case WM_MOUSEWHEEL: // Zoom with mouse wheel
	{
		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		// Zoom in/out based on wheel direction
		cameraDistance -= (float)zDelta / 10.0f;

		// Clamp the distance
		if (cameraDistance < 20.0f) cameraDistance = 20.0f;
		if (cameraDistance > 500.0f) cameraDistance = 500.0f;

		InvalidateRect(hWnd, NULL, FALSE);
	}
	break;

	// A menu command
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			// Exit the program
		case ID_FILE_EXIT:
			DestroyWindow(hWnd);
			break;

			// Display the about box
		case ID_HELP_ABOUT:
			DialogBox(hInstance,
				MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
				hWnd,
				(DLGPROC)AboutDlgProc);
			break;
		}
	}
	break;


	default:   // Passes it on if unproccessed
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
