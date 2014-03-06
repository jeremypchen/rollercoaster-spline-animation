/*
  CSCI 480
  Assignment 2
  Jeremy Chen
  7972623143
  */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <pic.h>
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

// declaring textures and jpg names
static GLuint ground_texture;
static GLuint sky_texture;
Pic *ground;
Pic *sky;

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

int g_iMenuId; // for menu purposes

double time_of_car = 0; // the u-value in the Catmull-Rom equation
int current_frame = 0;
int starting_segment = 0; // first seg_of_spline
int seg_of_spline = 0; // seg_of_spline of spline

point up_vector; //up vector for car
point rider_point; //where the car is
point lookat_point; //where it will be, shortly

// read splines from a file
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
    g_Splines[j].points = (point *)malloc(iLength * sizeof(point));
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

// function to configure quit and restart option when you right-click
void menu(int x){
  switch(x){
    case 0:
      exit(1);
      break;
    case 1: // restart
      current_frame = 0; 
      time_of_car = 0;
      seg_of_spline = starting_segment;
      break;
  }
}

void doIdle(){
  // Make the screen update
  glutPostRedisplay();
}

void normal(point *point){ // function to normalize a vector (divide by distance)
  double denom = sqrt(pow(point->x,2)+pow(point->y,2)+pow(point->z,2)); // distance formula of a vector
  point->x = point->x/denom; // divide by distance for each component
  point->y = point->y/denom;
  point->z = point->z/denom;
}

void cross_product(point *point1, point *point2, point *cross){ // function to find the cross product of point1 and point2
    double x = point1->y*point2->z - point1->z*point2->y; // determinant
    double y = point1->z*point2->x - point1->x*point2->z;
    double z = point1->x*point2->y - point1->y*point2->x;
    cross->x = x; // output in a new point called cross, each component
    cross->y = y; 
    cross->z = z;
}

// interpolating using the four points, outputting in "output", and using time u
void cm_equation(point *point1, point *point2, point *point3, point *point4, point *output, double u){
  // this is multiplying by the basis matrix, with s=0.5
  double row_4_x = 1.0*point2->x;
  double row_3_x = -0.5*point1->x + 0.5*point3->x;
  double row_2_x = 1.0*point1->x + -2.5*point2->x + 2.0*point3->x + -0.5*point4->x;
  double row_1_x = -0.5*point1->x + 1.5*point2->x + -1.5*point3->x + 0.5*point4->x;

  double row_4_y = 1.0*point2->y;
  double row_3_y = -0.5*point1->y + 0.5*point3->y;
  double row_2_y = 1.0*point1->y + -2.5*point2->y + 2.0*point3->y + -0.5*point4->y;
  double row_1_y = -0.5*point1->y + 1.5*point2->y + -1.5*point3->y + 0.5*point4->y;

  double row_4_z = 1.0*point2->z;
  double row_3_z = -0.5*point1->z + 0.5*point3->z;
  double row_2_z = 1.0*point1->z + -2.5*point2->z + 2.0*point3->z + -0.5*point4->z;
  double row_1_z = -0.5*point1->z + 1.5*point2->z + -1.5*point3->z + 0.5*point4->z;

  // multiply results by the equation u^3 + u^2 + u + 1
  output->x = row_1_x*(pow(u,3)) + row_2_x*(pow(u,2)) + row_3_x*u + row_4_x;
  output->y = row_1_y*(pow(u,3)) + row_2_y*(pow(u,2)) + row_3_y*u + row_4_y;
  output->z = row_1_z*(pow(u,3)) + row_2_z*(pow(u,2)) + row_3_z*u + row_4_z;
}

void camera()
{  
  point point1, point2;
  if (current_frame == 0) { // if we are at the start
    int length_minus_1 = g_Splines[0].numControlPoints-1;
    int length = g_Splines[0].numControlPoints;

    // interpolate and get the RIDER'S point
    cm_equation(&g_Splines[0].points[(length_minus_1)%length],
                &g_Splines[0].points[(length_minus_1+1)%length],
                &g_Splines[0].points[(length_minus_1+2)%length],
                &g_Splines[0].points[(length_minus_1+3)%length],
                &rider_point, 0.0); // output is in rider_point

    // interpolate and get the point where we will look at
    cm_equation(&g_Splines[0].points[(length_minus_1)%length],
                &g_Splines[0].points[(length_minus_1+1)%length],
                &g_Splines[0].points[(length_minus_1+2)%length],
                &g_Splines[0].points[(length_minus_1+3)%length],
                &lookat_point, 0.5); // notice the time u is slightly ahead, output is in rider_point
  }

  int previous = seg_of_spline - 1; // previous spline segment

  time_of_car += .001; // increment the time (increasing this will increase the speed of the camera)

  if (time_of_car > 1.0){ // if we hit the next spline segment, then update our segment
    seg_of_spline++;
    time_of_car = 0;
  }

  if (previous < 0) previous += g_Splines[0].numControlPoints; // if we're at the start, make previous positive and at the end of numControlPoints

  // interpolate and get the RIDER'S point
  cm_equation(&g_Splines[0].points[previous%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(previous+1)%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(previous+2)%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(previous+3)%g_Splines[0].numControlPoints],
            &rider_point, time_of_car);

  if (time_of_car+.1 <= 1.0) { // if we're on the same spline segment, look ahead a little
    // interpolate and get the point where we will look at
    cm_equation(&g_Splines[0].points[previous%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(previous+1)%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(previous+2)%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(previous+3)%g_Splines[0].numControlPoints],
            &lookat_point, time_of_car+.1); // time is ahead a little bit with the +.1

  } else { // if we're looking at the next spline segment, figure out that segment
    int newSegment = seg_of_spline+1;
    if (newSegment == g_Splines[0].numControlPoints) newSegment = 0;

    int newPrevious = newSegment - 1;
    if (newPrevious < 0) newPrevious += g_Splines[0].numControlPoints;

    // interpolate and get the point where we will look at
    cm_equation(&g_Splines[0].points[newPrevious%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(newPrevious+1)%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(newPrevious+2)%g_Splines[0].numControlPoints],
            &g_Splines[0].points[(newPrevious+3)%g_Splines[0].numControlPoints],
            &lookat_point,
            time_of_car-0.9);
  }

  // figure out the up vector
  point1 = rider_point;
  point2.x = rider_point.x - lookat_point.x;
  point2.y = rider_point.y - lookat_point.y;
  point2.z = rider_point.z - lookat_point.z;
  normal(&point1); // normalize these two to get tangent vector
  normal(&point2); // and normal vector
  cross_product(&point2, &point1, &up_vector); // cross product to find the binormal vector, which acts as our up-vector
}

void draw(point *point1, point *point2, point *point3, point *point4){ // draw the segment
  point *temp_point; temp_point = (point *)malloc(sizeof(point)); // make a temporary point

  cm_equation(point1, point2, point3, point4, temp_point, 0); // interpolate to get the location of the point

  for (double i=0; i<=500; i++){ // 500 represents the number of vertices that will be drawn
    double u = i/500; // the more drawn, the curvier the lines will be, but more lag
    glVertex3f(temp_point->x, temp_point->y, temp_point->z); // draw one vertex
    cm_equation(point1, point2, point3, point4, temp_point, u); // interpolate again to move ahead a bit
    glVertex3f(temp_point->x, temp_point->y, temp_point->z); // draw another vertex
  }

  free(temp_point); // free up the memory we just allocated
}

void display(){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the buffer
  glLoadIdentity();

  current_frame++; // increment the current frame

  camera(); // get the up-vector, rider_point and lookat_point

  rider_point.x += 1.15*up_vector.x; // prop the camera up slightly
  rider_point.y += 1.15*up_vector.y;
  rider_point.z += 1.15*up_vector.z;

  lookat_point.x += up_vector.x; // make the lookat_point slightly higher
  lookat_point.y += up_vector.y;
  lookat_point.z += up_vector.z;
  
  gluLookAt(0, 0, 0, // ensure that the sky texture stays in place
            lookat_point.x - rider_point.x, lookat_point.y - rider_point.y, (lookat_point.z - rider_point.z),
            up_vector.x, up_vector.y, up_vector.z);
  glScalef(300, 300, 300); // scale it so that it fits the cube properly

  /*DRAW THE SKY*/
  glBindTexture(GL_TEXTURE_2D, sky_texture);
  glDisable(GL_LIGHTING);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glEnable(GL_TEXTURE_2D);

  glBegin(GL_QUADS); // use quads and make SIX faces like a cube
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);

    glTexCoord2f(0, 0); glVertex3f(-1, -1, 1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, 1);

    glTexCoord2f(0, 0); glVertex3f(-1, 1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, 1, -1);

    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, -1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);

    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);
    glTexCoord2f(1, 1); glVertex3f(-1, 1, 1);
    glTexCoord2f(1, 0); glVertex3f(-1, 1, -1);

    glTexCoord2f(0, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(0, 2); glVertex3f(1, -1, 1);
    glTexCoord2f(2, 2); glVertex3f(1, 1, 1);
    glTexCoord2f(2, 0); glVertex3f(1, 1, -1);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_LIGHTING);

  glLoadIdentity();

  gluLookAt(rider_point.x, rider_point.y, rider_point.z,
          lookat_point.x, lookat_point.y, lookat_point.z,
          up_vector.x, up_vector.y, up_vector.z); // camera to look at where the coaster is going

  GLfloat light0_pos[] = {-45, -45, 15, 1}; // set up the two different lights
  GLfloat light1_pos[] = {45, 45, 15, 1}; // second light
  
  glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
  glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);

  glDisable(GL_LIGHTING);

  /*DRAW TRACK*/
  glLineWidth(18); // make the width of the lines 18
  glBegin(GL_LINES); // use lines
    for (int i=0; i<g_iNumOfSplines; i++) { // do it for the two splines in track.txt
      int l = g_Splines[i].numControlPoints;
      for (int j=0; j<l; j++) {
        glColor3f(10.0*i,10.0,10.0*j); // color it differently depending which spline you're on
          draw(&(g_Splines[i].points[j%l]), // call the draw function
                      &(g_Splines[i].points[(j+1)%l]),
                      &(g_Splines[i].points[(j+2)%l]),
                      &(g_Splines[i].points[(j+3)%l]));
      }
    }
  glEnd();

  glEnable(GL_LIGHTING);

  /*DRAW GROUND*/
  glBindTexture(GL_TEXTURE_2D, ground_texture);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS); // use quads
    for (int i =-400; i<400; i+=30){ // will cover large area
      for (int j=-400; j<400; j+=30){
        glTexCoord2f(0, 0); glVertex3f(i, j, -10); // four points for quads
        glTexCoord2f(0, 2); glVertex3f(i, j+30, -10); // make it a little lower than 0-plane
        glTexCoord2f(2, 2); glVertex3f(i+30, j+30, -10);
        glTexCoord2f(2, 0); glVertex3f(i+30, j, -10);
      }
    }
  glEnd();
  glDisable(GL_TEXTURE_2D);  

  glLoadIdentity();

  glutSwapBuffers();
}

void init_textures(){ // read in texture files and create the textures
  ground = jpeg_read("ground.jpg", NULL); // read in the ground picture

  glGenTextures(1, &ground_texture); // create ground texture
  glBindTexture(GL_TEXTURE_2D, ground_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  // texture is in 2D, 0 detail, RGB mode, x-size, y-size, 0 border, RGB data, unsigned byte, picture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ground->nx, ground->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, ground->pix);

  sky = jpeg_read("sky.jpg", NULL); // read in the sky picture

  glGenTextures(1, &sky_texture); // create sky texture
  glBindTexture(GL_TEXTURE_2D, sky_texture); 

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  // texture is in 2D, 0 detail, RGB mode, x-size, y-size, 0 border, RGB data, unsigned byte, picture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sky->nx, sky->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, sky->pix);
}

void myinit(){ // initialization
  glClearColor(0,0,0,0); // clear to black color

  glShadeModel(GL_SMOOTH); // shading is smooth
  glEnable(GL_TEXTURE_2D);
  init_textures(); // call the init_textures function

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_AUTO_NORMAL);

  GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0}; // diffuse lighting effect
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);

  glEnable(GL_LIGHTING); // enable the lighting
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);

  glViewport(0,0,640,480); // set up viewport to 640x480
  glMatrixMode(GL_PROJECTION); // projection mode
  glLoadIdentity();
  gluPerspective(45,1.333,0.1,1000); // viewing angle of 45 degrees, 1.333 comes from 640/480

  glMatrixMode(GL_MODELVIEW); // modelview mode
  glLoadIdentity();  

  glEnable(GL_POLYGON_SMOOTH); // smooth polygons and lines
  glEnable(GL_LINE_SMOOTH);
}

int main (int argc, char ** argv) { // main function
  if (argc<2) {  
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }

  loadSplines(argv[1]); // load up the splines

  // used to get the second spline, offset it to get two tracks
  for (int i=0; i<g_Splines[1].numControlPoints; i++){ // load up the second spline
    double x_offset = g_Splines[1].points[i].x;
    double y_offset = g_Splines[1].points[i].y;
    double z_offset = g_Splines[1].points[i].z;
    double normal = sqrt(pow(x_offset,2)+pow(y_offset,2)+pow(z_offset,2)); // normalize 
    x_offset = x_offset/normal*-1.1; // to offset
    y_offset = y_offset/normal*-1.1;
    z_offset = z_offset/normal*-1.1;
    g_Splines[1].points[i].x = g_Splines[0].points[i].x + x_offset; // offset slightly from the original track
    g_Splines[1].points[i].y = g_Splines[0].points[i].y + y_offset;
    g_Splines[1].points[i].z = g_Splines[0].points[i].z + z_offset;
  }

  glutInit(&argc,argv);
  
  glutInitDisplayMode(GLUT_DEPTH | GLUT_RGBA | GLUT_DOUBLE);
  glutInitWindowPosition(200,200); // window position on screen at 200,200  
  glutInitWindowSize(640, 480); // window size is 640x480
  glutCreateWindow("CSCI480 Assignment 2 - Jeremy Chen"); // window title
  
  glutDisplayFunc(display);
  
  g_iMenuId = glutCreateMenu(menu); // create menu
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit", 0);
  glutAddMenuEntry("Restart",1);
  glutAttachMenu(GLUT_RIGHT_BUTTON); // use right-button to get menu
  
  glutIdleFunc(doIdle);
  
  myinit();
  
  glutMainLoop();
}