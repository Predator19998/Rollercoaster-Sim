/*
  CSCI 420
  Assignment 2
  Jerry Sivaram Reddy Rajasimha Reddy
 */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "pic.h"
#include <math.h>

/* represents one control point along the spline */
struct point {
   double x;
   double y;
   double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

point normalize(point p){
	double length = sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.z, 2));
	point r;
	r.x = p.x / length;
	r.y = p.y / length;
	r.z = p.z / length;
	return r;
}
point crossProduct(point a, point b){
	point r;
	r.x = a.y * b.z - a.z * b.y;
	r.y = a.z * b.x - a.x * b.z;
	r.z = a.x * b.y - a.y * b.x;
	return r;
}

point addVectors(point a, point b){
	point r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;
	return r;
}

point scalarMultiply(double s, point a){
	point r;
	r.x = a.x * s;
	r.y = a.y * s;
	r.z = a.z * s;
	return r;
}


struct point CatmullRoll(float t, struct point p1, struct point p2, struct point p3, struct point p4)
{

	float t2 = t*t;
	float t3 = t*t*t;
	struct point v; // Interpolated point
		
	/* Catmull Rom spline Calculation */

	v.x = ((-t3 + 2*t2-t)*(p1.x) + (3*t3-5*t2+2)*(p2.x) + (-3*t3+4*t2+t)* (p3.x) + (t3-t2)*(p4.x))/2;
	v.y = ((-t3 + 2*t2-t)*(p1.y) + (3*t3-5*t2+2)*(p2.y) + (-3*t3+4*t2+t)* (p3.y) + (t3-t2)*(p4.y))/2;
  v.z = ((-t3 + 2*t2-t)*(p1.z) + (3*t3-5*t2+2)*(p2.z) + (-3*t3+4*t2+t)* (p3.z) + (t3-t2)*(p4.z))/2;
	return v;	
}

struct point CatmullRollGradient(float t, struct point p1, struct point p2, struct point p3, struct point p4)
{

	float t2 = t*t;
	float t3 = t*t*t;
	struct point v; // Interpolated point
		
	/* Catmull Rom spline gradient Calculation */

	v.x = ((-3*t2 + 4*t - 1)*(p1.x) + (9*t2-10*t)*(p2.x) + (-9*t2+8*t+1)* (p3.x) + (3*t2-2*t)*(p4.x))/2;
	v.y = ((-3*t2 + 4*t - 1)*(p1.y) + (9*t2-10*t)*(p2.y) + (-9*t2+8*t+1)* (p3.y) + (3*t2-2*t)*(p4.y))/2;
  v.z = ((-3*t2 + 4*t - 1)*(p1.z) + (9*t2-10*t)*(p2.z) + (-9*t2+8*t+1)* (p3.z) + (3*t2-2*t)*(p4.z))/2;
	return v;	
}

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

/* Spline infos*/
struct point * splinePoints;
struct point * splineTangents;
struct point * splineNormals;
struct point * splineBiNormals;
int numOfSplinePoints;
int cameraCoord = 0;
double SECTION_GAP = -0.5;
bool startCoaster = false;

GLboolean stop = false;

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* see <your pic directory>/pic.h for type Pic */
Pic * g_pHeightData;

char * screenshot_name = NULL;

//heightmap

char * heightmap = "heightmap/OhioPyle-768.jpg";
int width;
int height;
float* vertices = 0;
int* indices = 0;
float* color = 0;

int half;

GLuint texture[6];
float* texVertices = 0;
char * landScapeFilename = "Textures/snow.jpg";
//rock-mountain-512x512

char * skyFilename = "Textures/skybox_nz.jpeg";
char * railFilename = "Textures/metal.jpg";
char * bisectionFilename = "Textures/wood.jpeg";


//Get total count of the vertices for the traingular strips
int getVerticesCount( int width, int height ) {
  return 3 * (width * height);
}

//Get total number of the indices required for the heightmap
int getIndicesCount( int width, int height ) {
  return ((width*height) + (width-1)*(height-2));
}

//Store the vertices coordinates
float* getVertices( int width, int height ) {
    
    half = height/2;
    
    if ( vertices ) return vertices;

    vertices = new float[ getVerticesCount( width, height ) ];
    texVertices = new float[ 2 * getVerticesCount( width, height ) / 3];
    int i = 0;
    int j = 0;
    for ( int row=0; row<height; row++ ) {
        for ( int col=0; col<width; col++ ) {
            vertices[i++] = (float) col - half;
            //printf("%f,",(float)PIC_PIXEL(g_pHeightData,row,col,0));
            vertices[i++] = (float) PIC_PIXEL(g_pHeightData,col,row,0)/4;
            //vertices[i++] = 0.0f;
            vertices[i++] = (float) row - half;
            texVertices[j++] = (float) col/64;
            texVertices[j++] = (float) row/64;
        }
    }

    return vertices;
}

//Store the indices
int* getIndices( int width, int height ) {
    if ( indices ) return indices;

    indices = new int[ getVerticesCount( width, height ) ];
    int i = 0;

    for ( int row=0; row<height-1; row++ ) {
        if ( (row&1)==0 ) { //even rows
            for ( int col=0; col<width; col++ ) {
                indices[i++] = col + row * width;
                indices[i++] = col + (row+1) * width;
            }
        } else { //odd rows
            for ( int col=width-1; col>0; col-- ) {
                indices[i++] = col + (row+1) * width;
                indices[i++] = col - 1 + row * width;
            }
        }
    }

    return indices;
}

//loading splines
int loadSplines(char *argv) {
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;


  /* load the track file */
  fileList = fopen(argv, "r");
  if (fileList == NULL) {
    printf ("can't open file\n");
    exit(1);
  }
  
  /* stores the number of splines in a global variable */
  fscanf(fileList, "%d", &g_iNumOfSplines);

  g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

  /* reads through the spline files */
  for (j = 0; j < g_iNumOfSplines; j++) {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) {
      printf ("can't open file\n");
      exit(1);
    }

    /* gets length for spline file */
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    /* allocate memory for all the points */
    g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
    g_Splines[j].numControlPoints = iLength;

    /* saves the data to the struct */
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &g_Splines[j].points[i].x, 
	   &g_Splines[j].points[i].y, 
	   &g_Splines[j].points[i].z) != EOF) {
      i++;
    }
  }

  free(cName);

  return 0;
}


/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
  int i, j;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

void reshape (int width, int height) {
glViewport(0, 0, (GLsizei)width, (GLsizei)height); //Set our viewport to the size of our window
glMatrixMode(GL_PROJECTION); //Switch to the projection matrix so that we can manipulate how our scene is viewed
glLoadIdentity(); //Reset the projection matrix to the identity matrix so that we don't get any artifacts (cleaning up)
gluPerspective(90, (GLfloat)width / (GLfloat)height, 0.01, 1000.0); //Set the Field of view angle (in degrees), the aspect ratio of our window, and the near and far planes
glMatrixMode(GL_MODELVIEW); //Switch back to the model view matrix, so that we can start drawing shapes correctly
}

void myinit()
{
  glGenTextures(6, texture);
  Pic * img;
  img = jpeg_read(landScapeFilename, NULL);
  glBindTexture(GL_TEXTURE_2D, texture[0]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->nx, img->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, &img->pix[0]);
  pic_free(img);

  img = jpeg_read(skyFilename, NULL);
  glBindTexture(GL_TEXTURE_2D, texture[1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->nx, img->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, &img->pix[0]);
  pic_free(img);

  img = jpeg_read(railFilename, NULL);
  glBindTexture(GL_TEXTURE_2D, texture[2]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->nx, img->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, &img->pix[0]);
  pic_free(img);

  img = jpeg_read(bisectionFilename, NULL);
  glBindTexture(GL_TEXTURE_2D, texture[3]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->nx, img->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, &img->pix[0]);
  pic_free(img);

  GLfloat light_ambient[] = {1.0, 1.0, 1.0, 1.0};
  GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
  GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
  GLfloat light_position[] = { 300.0, 300.0, 300.0, 1.0 };
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  //glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,
    GL_TRUE);

  // enable depth buffering
  glEnable(GL_DEPTH_TEST);
  // interpolate colors during rasterization
  glShadeModel(GL_SMOOTH);
  
  /* setup gl view here */
  glClearColor(0.98, 0.72, 0.07, 1.0);
}

void movement(){
  
  //glTranslatef(0.0f,-100.0f,0.0f); // Move the heightmap down so that it is visible in the begining
  //glRotatef(90,0.0 , 1.0, 0.0);
  
  //Rotate the heightmap
  glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
  
  //Translate the heightmap
  glTranslatef(g_vLandTranslate[0], 0.0 , 0.0);
  glTranslatef(0.0, g_vLandTranslate[1] , 0.0);
  glTranslatef(0.0, 0.0, g_vLandTranslate[2]);
  
  //Scaling the heightmap
  glScalef(g_vLandScale[0], 1.0 ,1.0);
  glScalef(1.0, g_vLandScale[1] , 1.0);
  glScalef(1.0, 1.0 ,g_vLandScale[2]);
}

//store the splines 
void drawSpline() {
  int controlPoints = g_Splines[0].numControlPoints;
  numOfSplinePoints = controlPoints * (1/0.0002);
  double j = 0.0002;
  splinePoints = (struct point *)malloc(numOfSplinePoints * sizeof(struct point));
  splineTangents = (struct point *)malloc(numOfSplinePoints * sizeof(struct point));
  splineNormals = (struct point *)malloc(numOfSplinePoints * sizeof(struct point));
  splineBiNormals = (struct point *)malloc(numOfSplinePoints * sizeof(struct point));

  int index = 0;

  for(int i = 0 ; i < controlPoints - 3 ; i++ ) {

    //printf("%f,%f,%f\n",g_Splines[0].points[i].x,g_Splines[0].points[i].y,g_Splines[0].points[i].z);

    for(double t = 0 ; t < 1 ; t+=j){
      struct point p0 = g_Splines[0].points[i];
      struct point p1 = g_Splines[0].points[i+1];
      struct point p2 = g_Splines[0].points[i+2];
      struct point p3 = g_Splines[0].points[i+3];

      splinePoints[index] = CatmullRoll(t,p0,p1,p2,p3);
      splineTangents[index] = CatmullRollGradient(t,p0,p1,p2,p3);
      if (index == 0){
				point V0;
				V0.x = 0.0; V0.y = 0.0; V0.z = -1.0;
				splineNormals[index] = normalize(crossProduct(splineTangents[index], V0));
				splineBiNormals[index] = normalize(crossProduct(splineTangents[index], splineNormals[index]));
			}
			else {
				point previousBinormal = splineBiNormals[index - 1];
				splineNormals[index] = normalize(crossProduct(previousBinormal, splineTangents[index]));
				splineBiNormals[index] = normalize(crossProduct(splineTangents[index], splineNormals[index]));
			}

      index++;
    }
  }
  numOfSplinePoints = index;
}

//Generate the required heightmap
void renderHeightmap() {

    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glEnable(GL_TEXTURE_2D);
    glEnableClientState( GL_VERTEX_ARRAY );
	  glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glVertexPointer( 3, GL_FLOAT, 0, getVertices( width, height ) );
    //printf("loading textures\n");
    glTexCoordPointer(2, GL_FLOAT, 0, texVertices);

    glDrawElements( GL_TRIANGLE_STRIP, getIndicesCount( width, height ), GL_UNSIGNED_INT, getIndices( width, height ));

    //glDrawArrays(GL_POINTS,0,getVerticesCount(width,height));
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisable(GL_TEXTURE_2D);
}

//generating sky dome
void drawHalfSphere(int scaley, int scalex, GLfloat r) {
   int i, j;
   GLfloat v[scalex*scaley][3];

   for (i=0; i<scalex; ++i) {
     for (j=0; j<scaley; ++j) {
       v[i*scaley+j][0]=r*cos(j*2*M_PI/scaley)*cos(i*M_PI/(2*scalex));
       v[i*scaley+j][1]=r*sin(i*M_PI/(2*scalex));
       v[i*scaley+j][2]=r*sin(j*2*M_PI/scaley)*cos(i*M_PI/(2*scalex));
     }
   }

    glEnable(GL_TEXTURE_2D);
    glEnableClientState( GL_VERTEX_ARRAY );
	  glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glBegin(GL_QUADS);
     for (i=0; i<scalex-1; ++i) {
       for (j=0; j<scaley; ++j) {
         glTexCoord2f(v[i*scaley+j][0]/1024, v[i*scaley+j][1]/1024);glVertex3fv(v[i*scaley+j]);
         glTexCoord2f(v[i*scaley+(j+1)%scaley][0]/1024, v[i*scaley+(j+1)%scaley][1]/1024);glVertex3fv(v[i*scaley+(j+1)%scaley]);
         glTexCoord2f(v[(i+1)*scaley+(j+1)%scaley][0]/1024, v[(i+1)*scaley+(j+1)%scaley][1]/1024);glVertex3fv(v[(i+1)*scaley+(j+1)%scaley]);
         glTexCoord2f(v[(i+1)*scaley+j][0]/1024, v[(i+1)*scaley+j][1]/1024);glVertex3fv(v[(i+1)*scaley+j]);
       }
     }
   glEnd();
   glDisable(GL_TEXTURE_2D);
 }

//Create rail section
 void drawRailSection(point v0, point v1, point v2, point v3, point v4, point v5, point v6, point v7){

  glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//Top
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);glVertex3f(v1.x, v1.y, v1.z);
	glTexCoord2f(0,1);glVertex3f(v2.x, v2.y, v2.z);
	glTexCoord2f(1,0);glVertex3f(v6.x, v6.y, v6.z);
	glTexCoord2f(1,1);glVertex3f(v5.x, v5.y, v5.z);
	glEnd();

	//Bottom
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);glVertex3f(v0.x, v0.y, v0.z);
	glTexCoord2f(0,1);glVertex3f(v3.x, v3.y, v3.z);
	glTexCoord2f(1,0);glVertex3f(v7.x, v7.y, v7.z);
	glTexCoord2f(1,1);glVertex3f(v4.x, v4.y, v4.z);
	glEnd();

	//Left 
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);glVertex3f(v2.x, v2.y, v2.z);
	glTexCoord2f(0,1);glVertex3f(v6.x, v6.y, v6.z);
	glTexCoord2f(1,0);glVertex3f(v7.x, v7.y, v7.z);
	glTexCoord2f(1,1);glVertex3f(v3.x, v3.y, v3.z);
	glEnd();

	//Right
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);glVertex3f(v1.x, v1.y, v1.z);
	glTexCoord2f(0,1);glVertex3f(v5.x, v5.y, v5.z);
	glTexCoord2f(1,0);glVertex3f(v4.x, v4.y, v4.z);
	glTexCoord2f(1,1);glVertex3f(v0.x, v0.y, v0.z);
	glEnd();

  glDisable(GL_TEXTURE_2D);
}

//Create rail crossection
void drawCrossSection(point v0, point tangent, point normal, point binormal, double sectionGap, double size = 0.03){

  glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  
	//glColor3f(0.542, 0.30, 0.07);

	point v3 = addVectors(v0, scalarMultiply(sectionGap * 1.2, binormal));
	point v1 = addVectors(v0, scalarMultiply(size, normal));
	point v2 = addVectors(v3, scalarMultiply(size, normal));

	//Front
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);glVertex3f(v1.x, v1.y, v1.z);
	glTexCoord2f(0,1);glVertex3f(v2.x, v2.y, v2.z);
	glTexCoord2f(1,0);glVertex3f(v3.x, v3.y, v3.z);
	glTexCoord2f(1,1);glVertex3f(v0.x, v0.y, v0.z);
	glEnd();

  glDisable(GL_TEXTURE_2D);  
}

//generating support structure
void drawSupportStruture(point v0, point tangent, point normal, point binormal, double sectionGap, double size = 0.03){

  glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  
	//glColor3f(0.542, 0.30, 0.07);

	point v3 = addVectors(v0, scalarMultiply(sectionGap * 1.2, binormal));
	point v1 = addVectors(v0, scalarMultiply(size, normal));
	point v2 = addVectors(v3, scalarMultiply(size, normal));

	//Front
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);glVertex3f(v1.x, -75.0 , v1.z);
	glTexCoord2f(0,1);glVertex3f(v2.x, -75.0 , v2.z);
	glTexCoord2f(1,0);glVertex3f(v3.x, v3.y, v3.z);
	glTexCoord2f(1,1);glVertex3f(v0.x, v0.y, v0.z);
	glEnd();

  glDisable(GL_TEXTURE_2D);  
}

/*Given a spline index and a scale value, draw the rail bisection*/
void drawBisection(int splineIndex, double scale, double sectionInterval = 0.5, int interval = 1, bool crossSection = false, bool supportStruture = false){
	//glColor3f(0.5, 0.5, 0.5);

	point tangent = normalize(splineTangents[splineIndex]);
	point points1[4];
	points1[2] = splinePoints[splineIndex];
	points1[1] = addVectors(splinePoints[splineIndex], scalarMultiply(scale, splineBiNormals[splineIndex]));
	points1[0] = addVectors(points1[1], scalarMultiply(-1 * scale, splineNormals[splineIndex]));
	points1[3] = addVectors(splinePoints[splineIndex], scalarMultiply(-scale, splineNormals[splineIndex])); 

	tangent = normalize(splineTangents[splineIndex + interval]);
	point points2[4];
	points2[2] = splinePoints[splineIndex + interval];
	points2[1] = addVectors(splinePoints[splineIndex + interval], scalarMultiply(scale, splineBiNormals[splineIndex + interval]));
	points2[0] = addVectors(points2[1], scalarMultiply(-1 * scale, splineNormals[splineIndex + interval]));
	points2[3] = addVectors(splinePoints[splineIndex + interval], scalarMultiply(-scale, splineNormals[splineIndex + interval]));

	//Draw left section
	drawRailSection(points1[0], points1[1], points1[2], points1[3], points2[0], points2[1], points2[2], points2[3]);

	//Draw Cross section
	if (crossSection){
		drawCrossSection(points1[0], tangent, splineNormals[splineIndex], splineBiNormals[splineIndex], sectionInterval);
	}

	//DrawCross support structure
	// if (supportStruture){
	// 	drawSupportStruture(points1[0], tangent, splineNormals[splineIndex], splineBiNormals[splineIndex], sectionInterval);
	// }

	//glColor3f(0.5, 0.5, 0.5);
	for (point & v : points1){
		v = addVectors(v, scalarMultiply(sectionInterval, splineBiNormals[splineIndex]));
	}
	for (point & v : points2){
		v = addVectors(v, scalarMultiply(sectionInterval, splineBiNormals[splineIndex + interval]));
	}

	//Draw right section
	drawRailSection(points1[0], points1[1], points1[2], points1[3], points2[0], points2[1], points2[2], points2[3]);
}

//Draw the rollercoaster onto the scene
void renderObjects(){

  //Basic phong shading
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

  for (int i = 0; i < numOfSplinePoints-10; i+=10){
		
		double s = 0.05;

		drawBisection(i, s, SECTION_GAP, 10, i%100 == 0, i%1000 == 0);
	}

  glEnd();
}

void cameraMovement(point splinePoint, point tangent, point normal, point binormal){
  point eye = addVectors(splinePoint,  scalarMultiply(0.2, normal));
	eye = addVectors(eye, scalarMultiply(SECTION_GAP / 2, binormal));
	point center = addVectors(eye, tangent);
	point up = normal;
	
	gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);
}

void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  drawSpline();
  glLoadIdentity();

  if (!startCoaster)
  cameraMovement(splinePoints[0], splineTangents[0], splineNormals[0], splineBiNormals[0]);

  if(startCoaster) {
    if (cameraCoord >= numOfSplinePoints)
      cameraCoord = 0;
    cameraMovement(splinePoints[cameraCoord], splineTangents[cameraCoord], splineNormals[cameraCoord], splineBiNormals[cameraCoord]);

    //Camera movement based on gravity
    if(abs(sqrt( 2 * 9.8 * ( 100 - splinePoints[cameraCoord+1].y ) / 10 )) < 10) cameraCoord+=50;
    else cameraCoord+= abs(sqrt( 2 * 9.8 * ( 100 - splinePoints[cameraCoord+1].y ) / 10 ));
  }
	//Update the camera

  glPushMatrix();
  movement();
  //glColor3f(1.0,1.0,1.0);
  glTranslatef(0.0,-75.0,0.0f);
  renderHeightmap();
  glPopMatrix();

  glPushMatrix();
  movement();
  //glColor3f(0.0,1.0,0.0);
  glTranslatef(0.0,-75.0,0.0f);
  drawHalfSphere(32,32,350);
  //glutSolidSphere(350,50,50);
  glPushMatrix();

  glPushMatrix();
  glTranslatef(0.0,75.0,0.0f);
  renderObjects();
  glPopMatrix();

  glutSwapBuffers();
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
  }
}

void doIdle()
{
  /* do some stuff... */

  /* make the screen update */
  glutPostRedisplay();
}

/* converts mouse drags into information about 
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.5;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.5;
      }
      if (g_iRightMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.5;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1]*0.5;
        g_vLandRotate[1] += vMouseDelta[0]*0.5;
      }
      if (g_iRightMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1]*0.5;
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iRightMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void keyboard(unsigned char key, int x, int y)
{
    if (key=='q' || key == 'Q')
        exit(0);

    if (key=='z')
      g_ControlState = ROTATE;
    if (key=='x')
      g_ControlState = SCALE;
    if (key=='c')
      g_ControlState = TRANSLATE;
  
  if (key=='s')
    if (screenshot_name != NULL) {
      saveScreenshot(screenshot_name);
    }

  if (key=='w')
    startCoaster = !startCoaster;  
}

int main (int argc, char ** argv)
{
  if (argc<2)
  {  
  printf ("usage: %s <trackfile>\n", argv[0]);
  exit(0);
  }

  g_pHeightData = jpeg_read(heightmap, NULL);
  if (!g_pHeightData)
  {
    printf ("error reading %s.\n", heightmap);
    //exit(1);
  }

  width = g_pHeightData->nx;
  height = g_pHeightData->ny;

  loadSplines(argv[1]);

  // screenshot_name = argv[2];
  
  glutInit(&argc,argv);
  
  /*
    create a window here..should be double buffered and use depth testing
  
    the code past here will segfault if you don't have a window set up....
    replace the exit once you add those calls.
  */
  
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  
  // set window size
  glutInitWindowSize(640, 480);
  
  // set window position
  glutInitWindowPosition(100, 100);
  
  // creates a window
  glutCreateWindow("Test");
  
  //exit(0);

  /* tells glut to use a particular display function to redraw */
  glutDisplayFunc(display);
  
  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit",0);
  glutAttachMenu(GLUT_MIDDLE_BUTTON);
  
  /* replace with any animate code */
  glutIdleFunc(doIdle);

  /* callback for mouse drags */
  glutMotionFunc(mousedrag);
  /* callback for idle mouse movement */
  glutPassiveMotionFunc(mouseidle);
  /* callback for mouse button changes */
  glutMouseFunc(mousebutton);
  
  glutKeyboardFunc(keyboard);//Keyboard functions for mac

  glutReshapeFunc(reshape);//To set up the camera for the scene
  
  /* do initialization */
  myinit();

  glutMainLoop();

  return 0;
}
