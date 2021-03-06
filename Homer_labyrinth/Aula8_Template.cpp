#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <AL/alut.h>
#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

//#ifdef USE_GLAUX
#ifdef USE_GLAUX
	#include <GL/glaux.h>
#endif

#include "mathlib.h"
#include "studio.h"
#include "mdlviewer.h"

#pragma comment (lib, "glaux.lib")    /* link with Win32 GLAUX lib usada para ler bitmaps */

// fun��o para ler jpegs do ficheiro readjpeg.c
extern "C" int read_JPEG_file(const char *, char **, int *, int *, int *);

#ifndef M_PI
		#define M_PI 3.1415926535897932384626433832795
#endif

#define RAD(x)          (M_PI*(x)/180)
#define GRAUS(x)        (180*(x)/M_PI)

#define	GAP					              25

#define MAZE_HEIGHT			          18
#define MAZE_WIDTH			          18

#define	OBJECTO_ALTURA		      0.4
#define OBJECTO_VELOCIDADE	      0.5
#define OBJECTO_ROTACAO		        5
#define OBJECTO_RAIO		      0.12
#define SCALE_HOMER               0.005
#define EYE_ROTACAO			        1

#ifdef USE_GLAUX
    #define NOME_TEXTURA_CUBOS        "Textura.bmp"
#else
    #define NOME_TEXTURA_CUBOS        "Textura.jpg"
#endif

#define NOME_TEXTURA_CHAO         "Chao.jpg"


#define NUM_TEXTURAS              2
#define ID_TEXTURA_CUBOS          0
#define ID_TEXTURA_CHAO           1

#define	CHAO_DIMENSAO		      10

#define NUM_JANELAS               2
#define JANELA_TOP                0
#define JANELA_NAVIGATE           1

using namespace std;

typedef struct teclas_t{
	GLboolean   up, down, left, right;
}teclas_t;

typedef struct pos_t{
    GLfloat    x,y,z;
}pos_t;

typedef struct objecto_t{
    pos_t    pos;  
    GLfloat  dir; 
    GLfloat  vel; 
}objecto_t;

typedef struct camera_t{
    pos_t    eye;  
    GLfloat  dir_long;  // longitude olhar (esq-dir)
    GLfloat  dir_lat;   // latitude olhar	(cima-baixo)
    GLfloat  fov;
}camera_t;

typedef struct ESTADO{
    camera_t      camera;
    GLint         timer;
    GLint         mainWindow,topSubwindow,navigateSubwindow;
    teclas_t      teclas;
    GLboolean     localViewer;
    GLuint        vista[NUM_JANELAS];
	ALuint		  buffer, source;
	ALboolean	  tecla_som;
}ESTADO;

typedef struct MODELO{
    GLuint        texID[NUM_JANELAS][NUM_TEXTURAS];
    GLuint        labirinto[NUM_JANELAS];
    GLuint        chao[NUM_JANELAS];
    objecto_t	    objecto;
    GLuint        xMouse;
    GLuint        yMouse;
    StudioModel   homer[NUM_JANELAS];   // Modelo Homer
    GLboolean     andar;
    GLuint        prev;
	time_t		  caiuAt; //hora a que o homer caiu
}MODELO;

/////////////////////////////////////
//variaveis globais

ESTADO estado;
MODELO modelo;

char mazedata[MAZE_HEIGHT][MAZE_WIDTH+1] = {
  "                  ",
  " ******* ******** ",
  " *       *      * ",
  " * * *** * *    * ",
  " * **  * ** * * * ",
  " *     *      * * ",
  " *          *** * ",
  " *           *  * ",
  " *     * *** **** ",
  " * *   *   *    * ",
  " *   ****  *    * ",
  " ********  **** * ",
  " *            * * ",
  " *     *      * * ",
  " ** ** *    *** * ",
  " *   *      *   * ",
  " *******  **** ** ",
  "                  "
};


////////////////////////////////////
/// Ilumina��o e materiais


void setLight()
{	
	GLfloat light_pos[4] =	{-5.0, 20.0, -8.0, 0.0};
	GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat light_specular[]=	{ 0.5f, 0.5f, 0.5f, 1.0f };
	
    // ligar ilumina��o
	glEnable(GL_LIGHTING);

	// ligar e definir fonte de luz 0
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,estado.localViewer);
}

void setMaterial()
{
	GLfloat mat_specular[]= { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat mat_shininess = 104;

    // cria��o autom�tica das componentes Ambiente e Difusa do material a partir das cores
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	
    // definir de outros par�metros dos materiais est�ticamente
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess); 
}


///////////////////////////////////
//// Redisplays

void redisplayTopSubwindow(int width, int height)
{
	// glViewport(botom, left, width, height)
	// define parte da janela a ser utilizada pelo OpenGL
	glViewport(0, 0, (GLint) width, (GLint) height);
	// Matriz Projeccao
	// Matriz onde se define como o mundo e apresentado na janela
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	gluPerspective(60,(GLfloat)width/height,.5,100);
	// Matriz Modelview
	// Matriz onde s�o realizadas as tranformacoes dos modelos desenhados
	glMatrixMode(GL_MODELVIEW);

}


void reshapeNavigateSubwindow(int width, int height)
{
	// glViewport(botom, left, width, height)
	// define parte da janela a ser utilizada pelo OpenGL
	glViewport(0, 0, (GLint) width, (GLint) height);
	// Matriz Projeccao
	// Matriz onde se define como o mundo e apresentado na janela
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	gluPerspective(estado.camera.fov,(GLfloat)width/height,0.1,50);
	// Matriz Modelview
	// Matriz onde s�o realizadas as tranformacoes dos modelos desenhados
	glMatrixMode(GL_MODELVIEW);
}

void reshapeMainWindow(int width, int height)
{
	GLint w,h;
    w = (width-GAP*3)*.5;
    h = (height-GAP*2);    
    glutSetWindow(estado.topSubwindow);
    glutPositionWindow(GAP, GAP);
    glutReshapeWindow(w, h);
    glutSetWindow(estado.navigateSubwindow);
    glutPositionWindow(GAP+w+GAP, GAP);
    glutReshapeWindow(w, h);

}

void strokeCenterString(char *str,double x, double y, double z, double s)
{
	int i,n;
	
	n = strlen(str);
	glPushMatrix();
	  glTranslated(x-glutStrokeLength(GLUT_STROKE_ROMAN,(const unsigned char*)str)*0.5*s,y-119.05*0.5*s,z);
	  glScaled(s,s,s);
	  for(i=0;i<n;i++)
		  glutStrokeCharacter(GLUT_STROKE_ROMAN,(int)str[i]);
	glPopMatrix();

}

/////////////////////////////////////
//Modelo


GLboolean detectaColisao(GLfloat nx,GLfloat nz)
{
	int l = (int)(nz + MAZE_HEIGHT / 2.0 + 0.5);
	int c = (int)(nx + MAZE_WIDTH / 2.0 + 0.5);

			if (mazedata[l][c] == '*')
			{
				return true;
			}
			else
			{
				return false;
			}
	return GL_FALSE;
}

/*
 * O glTexCoord2f vai buscar � imagem as texturas para "colar". Se fizermos as contas na imagem, os valores 0.25 0.5 e etc v�o buscar o quadrado
 * castanho para fazer os mapas.
 */
void desenhaPoligono(GLfloat a[], GLfloat b[], GLfloat c[], GLfloat  d[], GLfloat normal[], GLuint texID)
{
    glBegin(GL_POLYGON);
        glNormal3fv(normal);
		glTexCoord2f(0.5, 0.5);
        glVertex3fv(a);

		glTexCoord2f(0.25,0.5);
        glVertex3fv(b);
		
		glTexCoord2f(0.25,0.25);
        glVertex3fv(c);
		
		glTexCoord2f(0.5,0.25);
        glVertex3fv(d);
    glEnd();
}

// EXTRA
void desenhaEixos()
{
	// x
	glBegin(GL_LINES); 
	glColor3f(255, 0, 0);
	glVertex3f(4, 0, 0);
	glVertex3f(0, 0, 0);
	glEnd();

	// y
	glBegin(GL_LINES);
	glColor3f(0, 255, 0);
	glVertex3f(0, 4, 0);
	glVertex3f(0, 0, 0);
	glEnd();

	// z
	glBegin(GL_LINES);
	glColor3f(0, 0, 255);
	glVertex3f(0, 0, 4);
	glVertex3f(0, 0, 0);
	glEnd();
}

void desenhaCubo(GLuint texID)
{
  GLfloat vertices[][3] = { {-0.5,-0.5,-0.5}, 
                            {0.5,-0.5,-0.5}, 
                            {0.5,0.5,-0.5}, 
                            {-0.5,0.5,-0.5}, 
                            {-0.5,-0.5,0.5},  
                            {0.5,-0.5,0.5}, 
                            {0.5,0.5,0.5}, 
                            {-0.5,0.5,0.5}};

  GLfloat normais[][3] = {  { 0, 0, -1 },
							{ 0, -1, 0 },
							{ -1, 0, 0 },
							{ 0, -1, -1 },
							{ -1, -1, -1 },
							{ -1, -1, 0 },
							{ -1, 0, -1 },
							{ 0, 0, 0 }
                         };


  glBindTexture(GL_TEXTURE_2D, texID);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);


  desenhaPoligono(vertices[1],vertices[0],vertices[3],vertices[2],normais[0], texID);
  desenhaPoligono(vertices[2],vertices[3],vertices[7],vertices[6],normais[1], texID);
  desenhaPoligono(vertices[3],vertices[0],vertices[4],vertices[7],normais[2], texID);
  desenhaPoligono(vertices[6],vertices[5],vertices[1],vertices[2],normais[3], texID);
  desenhaPoligono(vertices[4],vertices[5],vertices[6],vertices[7],normais[4], texID);
  desenhaPoligono(vertices[5],vertices[4],vertices[0],vertices[1],normais[5], texID);

  //glBindTexture(GL_TEXTURE_2D, NULL);
}


void desenhaBussola(int width, int height)  // largura e altura da janela
{

  // Altera viewport e projec��o para 2D (copia de um reshape de um projecto 2D)

  //....

  // Blending (transparencias)
/*  glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_COLOR_MATERIAL);
*/

  //desenha bussola 2D

	//glColor3f(1,0.4,0.4);
	//strokeCenterString("N", 0, 20, 0 , 0.1); // string, x ,y ,z ,scale
	
	
  // rop�e estado
 /* glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
*/

  //rep�e projec��o chamando redisplay
  reshapeNavigateSubwindow(width, height);

}

void desenhaModeloDir(objecto_t obj,int width, int height)
{
  // Altera viewport e projec��o
//....

  // Blending (transparencias)
/*
  glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
*/

  //desenha Seta

  // rop�e estado
/*  glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
*/
  //rep�e projec��o chamando redisplay
  redisplayTopSubwindow(width, height);
}

void desenhaAngVisao(camera_t *cam)
{
    GLfloat ratio;
    ratio=(GLfloat)glutGet(GLUT_WINDOW_WIDTH)/glutGet(GLUT_WINDOW_HEIGHT); // propor��o 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glPushMatrix();
        glTranslatef(cam->eye.x,OBJECTO_ALTURA,cam->eye.z);
        glColor4f(0,0,1,0.2);
        glRotatef(GRAUS(cam->dir_long),0,1,0);

        glBegin(GL_TRIANGLES);
            glVertex3f(0,0,0);
            glVertex3f(5*cos(RAD(cam->fov*ratio*0.5)),0,-5*sin(RAD(cam->fov*ratio*0.5)));
            glVertex3f(5*cos(RAD(cam->fov*ratio*0.5)),0,5*sin(RAD(cam->fov*ratio*0.5)));
        glEnd();
    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void desenhaModelo()
{
    glColor3f(0,1,0);
    glutSolidCube(OBJECTO_ALTURA);
    glPushMatrix();
        glColor3f(1,0,0);
        glTranslatef(0,OBJECTO_ALTURA*0.75,0);
        glRotatef(GRAUS(estado.camera.dir_long-modelo.objecto.dir),0,1,0);
        glutSolidCube(OBJECTO_ALTURA*0.5);
    glPopMatrix();
}

void desenhaLabirinto(GLuint texID)
{
    // c�digo para desenhar o labirinto
	glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslatef(-MAZE_HEIGHT / 2.0, 0.5, -MAZE_WIDTH / 2.0);

		int z,x;

		for (z = 0; z <= MAZE_HEIGHT; z++)
		{
			for (x = 0; x <= MAZE_WIDTH; x++)
			{
				if (mazedata[z][x] == '*')
				{
					glPushMatrix();
					glTranslatef(x, 0, z);
					desenhaCubo(texID);
					glPopMatrix();
				}
			}
		}
	glPopMatrix();
}

#define STEP    1

void desenhaChao(GLfloat dimensao, GLuint texID)
{
    // c�digo para desenhar o ch�o
    GLfloat i,j;
    glBindTexture(GL_TEXTURE_2D, texID);

    glColor3f(0.5f,0.5f,0.5f);
    for(i=-dimensao;i<=dimensao;i+=STEP)
        for(j=-dimensao;j<=dimensao;j+=STEP)
        {
            glBegin(GL_POLYGON);
                glNormal3f(0,1,0);
                glTexCoord2f(1,1);
                glVertex3f(i+STEP,0,j+STEP);
                glTexCoord2f(0,1);
                glVertex3f(i,0,j+STEP);
                glTexCoord2f(0,0);
                glVertex3f(i,0,j);
                glTexCoord2f(1,0);
                glVertex3f(i+STEP,0,j);
            glEnd();
        }
    glBindTexture(GL_TEXTURE_2D, NULL);
}

/////////////////////////////////////
//navigateSubwindow

void motionNavigateSubwindow(int x, int y)
{

}


void mouseNavigateSubwindow(int button, int state, int x, int y)
{

}

void setNavigateSubwindowCamera(camera_t *cam, objecto_t obj)
{

  pos_t center;

	if(!estado.vista[JANELA_NAVIGATE])
	{
		// 3� pessoa
		cam->eye.x=obj.pos.x - cos(obj.dir);
		cam->eye.y=obj.pos.y+.2;
		cam->eye.z = obj.pos.z + sin(obj.dir);
		estado.camera.dir_long = obj.dir;
		center.x=obj.pos.x;
		center.y=obj.pos.y+.2;
		center.z=obj.pos.z;
	}
	else
	{
		// 1� pessoa
		cam->eye.x = obj.pos.x;
		cam->eye.y = obj.pos.y+.2;
		cam->eye.z = obj.pos.z;
		estado.camera.dir_long = obj.dir;
		center.x = obj.pos.x + cos(obj.dir);
		center.y = OBJECTO_ALTURA;
		center.z = obj.pos.z - sin(obj.dir);
	}

	gluLookAt(cam->eye.x,cam->eye.y,cam->eye.z,center.x,center.y,center.z,0,1,0);
}


void displayNavigateSubwindow()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glLoadIdentity();

	setNavigateSubwindowCamera(&estado.camera, modelo.objecto);
    setLight();

	glCallList(modelo.labirinto[JANELA_NAVIGATE]);
	glCallList(modelo.chao[JANELA_NAVIGATE]);

	if(!estado.vista[JANELA_NAVIGATE])
      {
        glPushMatrix();
            glTranslatef(modelo.objecto.pos.x,modelo.objecto.pos.y,modelo.objecto.pos.z);
            glRotatef(GRAUS(modelo.objecto.dir),0,1,0);
            glRotatef(-90,1,0,0);
            glScalef(SCALE_HOMER,SCALE_HOMER,SCALE_HOMER);
            mdlviewer_display(modelo.homer[JANELA_NAVIGATE]);
        glPopMatrix();
      }

	desenhaBussola(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));

	glutSwapBuffers();
}

/////////////////////////////////////
//topSubwindow
void setTopSubwindowCamera( camera_t *cam,objecto_t obj)
{
	cam->eye.x=obj.pos.x;
	cam->eye.z=obj.pos.z;
	if(estado.vista[JANELA_TOP])
		gluLookAt(obj.pos.x,CHAO_DIMENSAO*.2,obj.pos.z,obj.pos.x,obj.pos.y,obj.pos.z,0,0,-1);
	else
		gluLookAt(obj.pos.x,CHAO_DIMENSAO*2,obj.pos.z,obj.pos.x,obj.pos.y,obj.pos.z,0,0,-1);
}

void displayTopSubwindow()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    glLoadIdentity();
    setTopSubwindowCamera(&estado.camera,modelo.objecto);
    setLight();

	glCallList(modelo.labirinto[JANELA_TOP]);
	glCallList(modelo.chao[JANELA_TOP]);
	
    glPushMatrix();		
        glTranslatef(modelo.objecto.pos.x,modelo.objecto.pos.y,modelo.objecto.pos.z);
        glRotatef(GRAUS(modelo.objecto.dir),0,1,0);
        glRotatef(-90,1,0,0);
        glScalef(SCALE_HOMER,SCALE_HOMER,SCALE_HOMER);
        mdlviewer_display(modelo.homer[JANELA_TOP]);
    glPopMatrix();

	desenhaAngVisao(&estado.camera);
	desenhaModeloDir(modelo.objecto,glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
	glutSwapBuffers();
}


/////////////////////////////////////
//mainWindow

void redisplayAll(void)
{
    glutSetWindow(estado.mainWindow);
    glutPostRedisplay();
    glutSetWindow(estado.topSubwindow);
    glutPostRedisplay();
    glutSetWindow(estado.navigateSubwindow);
    glutPostRedisplay();
}

void displayMainWindow()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glutSwapBuffers();
}

time_t getCurrentTime(){
	time_t current;
	time(&current);
	return current;
}

void Timer(int value)
{
	ALint state;

	if(modelo.homer[JANELA_NAVIGATE].GetSequence()==20 && getCurrentTime() < modelo.caiuAt + 1 ){
		 glutTimerFunc(estado.timer, Timer, 0);
		 redisplayAll();
		 return ;
	}
	//else{
	//	modelo.homer[JANELA_NAVIGATE].SetSequence(0);
	//	modelo.homer[JANELA_TOP].SetSequence(0);
	//}
	GLfloat nx=0,nz=0;
    GLboolean andar=GL_FALSE;
	GLboolean cair=GL_FALSE;

	GLuint curr = glutGet(GLUT_ELAPSED_TIME);
  // calcula velocidade baseado no tempo passado
	float velocidade= modelo.objecto.vel*(curr - modelo.prev )*0.001;

	glutTimerFunc(estado.timer, Timer, 0);
	modelo.prev = curr;

	if(estado.teclas.up){
	// calcula nova posi��o nx,nz
		nx = modelo.objecto.pos.x + cos(modelo.objecto.dir) * velocidade;
		nz = modelo.objecto.pos.z - sin(modelo.objecto.dir) * velocidade;

		if(!detectaColisao(nx,nz)){
			modelo.objecto.pos.x=nx;
			modelo.objecto.pos.z=nz;
		}
		else{
			modelo.caiuAt = getCurrentTime();
			modelo.homer[JANELA_NAVIGATE].SetSequence(20);
			modelo.homer[JANELA_TOP].SetSequence(20);
			cair = GL_TRUE;
		}
		andar=GL_TRUE;
	}
	/*n�o � necess�rio um else para por o andar a false, porque a variavel � incializada a false*/
	if(estado.teclas.down){
		// calcula nova posi��o nx,nz
		nx = modelo.objecto.pos.x - cos(modelo.objecto.dir) * velocidade;
		nz = modelo.objecto.pos.z + sin(modelo.objecto.dir) * velocidade;

		if(!detectaColisao(nx,nz)){
			modelo.objecto.pos.x=nx;
			modelo.objecto.pos.z=nz;
		}
    andar=GL_TRUE;
	}
	
	if(estado.teclas.left){
		// rodar objecto
		modelo.objecto.dir += RAD(5);
    }
	if(estado.teclas.right){
		// rodar objecto
		modelo.objecto.dir -= RAD(5);
	}
	if (andar && cair==GL_FALSE && modelo.homer[JANELA_NAVIGATE].GetSequence() != 13) {
		modelo.homer[JANELA_NAVIGATE].SetSequence(13);
		modelo.homer[JANELA_TOP].SetSequence(13);
	} 
	if (!andar && cair==GL_FALSE && modelo.homer[JANELA_NAVIGATE].GetSequence() != 0) {
		modelo.homer[JANELA_NAVIGATE].SetSequence(0);
		modelo.homer[JANELA_TOP].SetSequence(0);
	}

	alGetSourcei(estado.source, AL_SOURCE_STATE, &state);
	if (estado.tecla_som)
	{
		if (state != AL_PLAYING)
			alSourcePlay(estado.source);
	}
	else
		if (state == AL_PLAYING)
			alSourceStop(estado.source);

// Sequencias - 0(parado) 3(andar) 20(choque)
//  modelo.homer[JANELA_NAVIGATE].GetSequence()  le Sequencia usada pelo homer
//  modelo.homer[JANELA_NAVIGATE].SetSequence()  muda Sequencia usada pelo homer

  redisplayAll();

}


void imprime_ajuda(void)
{
  printf("\n\nDesenho de um quadrado\n");
  printf("h,H - Ajuda \n");
  printf("******* Diversos ******* \n");
  printf("l,L - Alterna o calculo luz entre Z e eye (GL_LIGHT_MODEL_LOCAL_VIEWER)\n");
  printf("w,W - Wireframe \n");
  printf("f,F - Fill \n");
  printf("s,S - Solid. retira texturas. \n");
  printf("a,A - tocar/parar o som. \n");
  printf("******* Movimento ******* \n");
  printf("up  - Andar para a frente \n");
  printf("down- Andar para a tr�s \n");
  printf("left- Vira para a esquerda\n");
  printf("right- Vira para a direita\n");
  printf("******* Camara ******* \n");
  printf("F1 - Alterna camara da janela da Esquerda \n");
  printf("F2 - Alterna camara da janela da Direita \n");
  printf("PAGE_UP, PAGE_DOWN - Altera abertura da camara \n");
  printf("botao esquerdo + movimento na Janela da Direita altera o olhar \n");
  printf("ESC - Sair\n");
}


void Key(unsigned char key, int x, int y)
{
	switch (key) {
		case 27:
			exit(1);
			break;
    case 'h' :
    case 'H' :
                imprime_ajuda();
            break;
    case 'l':
    case 'L':
        estado.localViewer=!estado.localViewer;
        break;
    case 'w':
    case 'W':
          glutSetWindow(estado.navigateSubwindow);
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
          glDisable(GL_TEXTURE_2D);
          glutSetWindow(estado.topSubwindow);
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
          glDisable(GL_TEXTURE_2D);
        break;
	case 'S':
	case 's':
		glutSetWindow(estado.navigateSubwindow);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_TEXTURE_2D);
		glutSetWindow(estado.topSubwindow);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_TEXTURE_2D);
		break;
	case 'a':
	case 'A':
		estado.tecla_som = AL_TRUE;
		break;
	}

}
void SpecialKey(int key, int x, int y)
{
	switch (key) {
		case GLUT_KEY_UP: estado.teclas.up =GL_TRUE;
			break;
		case GLUT_KEY_DOWN: estado.teclas.down =GL_TRUE;
			break;
		case GLUT_KEY_LEFT: estado.teclas.left =GL_TRUE;
			break;
		case GLUT_KEY_RIGHT: estado.teclas.right =GL_TRUE;
			break;
		case GLUT_KEY_F1: estado.vista[JANELA_TOP]=!estado.vista[JANELA_TOP];
			break;
		case GLUT_KEY_F2: estado.vista[JANELA_NAVIGATE]=!estado.vista[JANELA_NAVIGATE];
			break;
		case GLUT_KEY_PAGE_UP: 
                  if(estado.camera.fov>10)
                  {
                    estado.camera.fov--;
                    glutSetWindow(estado.navigateSubwindow);
                    reshapeNavigateSubwindow(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
                    redisplayAll();
                  }
			break;
		case GLUT_KEY_PAGE_DOWN: 
                  if(estado.camera.fov<130)
                  {
                    estado.camera.fov++;
                    glutSetWindow(estado.navigateSubwindow);
                    reshapeNavigateSubwindow(glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
                    redisplayAll();
                  }
			break;
	}

}
// Callback para interaccao via teclas especiais (largar na tecla)
void SpecialKeyUp(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_UP: estado.teclas.up = GL_FALSE;
            break;
    case GLUT_KEY_DOWN: estado.teclas.down = GL_FALSE;
            break;
    case GLUT_KEY_LEFT: estado.teclas.left = GL_FALSE;
            break;
    case GLUT_KEY_RIGHT: estado.teclas.right = GL_FALSE;
            break;
  }
}

////////////////////////////////////
// Inicializa��es



void createDisplayLists(int janelaID)
{
	modelo.labirinto[janelaID]=glGenLists(2);
	glNewList(modelo.labirinto[janelaID], GL_COMPILE);
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT );
		  desenhaLabirinto(modelo.texID[janelaID][ID_TEXTURA_CUBOS]);
		glPopAttrib();
		desenhaEixos();

	glEndList();

	modelo.chao[janelaID]=modelo.labirinto[janelaID]+1;
	glNewList(modelo.chao[janelaID], GL_COMPILE);
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT );
		  desenhaChao(CHAO_DIMENSAO,modelo.texID[janelaID][ID_TEXTURA_CHAO]);
		glPopAttrib();
	glEndList();
}


///////////////////////////////////
/// Texturas


// S� para windows (usa biblioteca glaux)
#ifdef USE_GLAUX

AUX_RGBImageRec *LoadBMP(char *Filename)				// Loads A Bitmap Image
{
	FILE *File=NULL;									// File Handle

	if (!Filename)										// Make Sure A Filename Was Given
	{
		return NULL;									// If Not Return NULL
	}

	File=fopen(Filename,"r");							// Check To See If The File Exists

	if (File)											// Does The File Exist?
	{
		fclose(File);									// Close The Handle
		return auxDIBImageLoad(Filename);				// Load The Bitmap And Return A Pointer
	}

	return NULL;										// If Load Failed Return NULL
}
#endif

void createTextures(GLuint texID[])
{
    char *image;
    int w, h, bpp;

#ifdef USE_GLAUX
    AUX_RGBImageRec *TextureImage[1];					// Create Storage Space For The Texture

    memset(TextureImage,0,sizeof(void *)*1);           	// Set The Pointer To NULL
#endif

    glGenTextures(NUM_TEXTURAS,texID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#ifdef USE_GLAUX
    if (TextureImage[0]=LoadBMP(NOME_TEXTURA_CUBOS))
    {
        // Create MipMapped Texture
        glBindTexture(GL_TEXTURE_2D, texID[ID_TEXTURA_CUBOS]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);

        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);
    }
#else
    if (read_JPEG_file(NOME_TEXTURA_CUBOS, &image, &w, &h, &bpp))
    {
        // Create MipMapped Texture
        glBindTexture(GL_TEXTURE_2D, texID[ID_TEXTURA_CUBOS]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);

        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_RGB, GL_UNSIGNED_BYTE, image);
    }
#endif
    else
    {
        printf("Textura %s not Found\n",NOME_TEXTURA_CUBOS);
        exit(0);
    }

    if(	read_JPEG_file(NOME_TEXTURA_CHAO, &image, &w, &h, &bpp))
    {
        glBindTexture(GL_TEXTURE_2D, texID[ID_TEXTURA_CHAO]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_RGB, GL_UNSIGNED_BYTE, image);
    }else{
        printf("Textura %s not Found\n",NOME_TEXTURA_CHAO);
        exit(0);
    }
	glBindTexture(GL_TEXTURE_2D, NULL);
}


void init()
{
    GLfloat amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };

	estado.timer=100;	
	
	estado.camera.eye.x=0;
	estado.camera.eye.y=OBJECTO_ALTURA*2;
	estado.camera.eye.z=0;
	estado.camera.dir_long=0;
	estado.camera.dir_lat=0;
	estado.camera.fov=60;

	estado.localViewer=1;
	estado.vista[JANELA_TOP]=0;
	estado.vista[JANELA_NAVIGATE]=0;

	modelo.objecto.pos.x=0;
	modelo.objecto.pos.y=OBJECTO_ALTURA*.5;
	modelo.objecto.pos.z=0;
	modelo.objecto.dir=0;
	modelo.objecto.vel=OBJECTO_VELOCIDADE;

	modelo.xMouse=modelo.yMouse=-1;
    modelo.andar=GL_FALSE;

	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);  // por causa do Scale ao Homer

    if(glutGetWindow()==estado.mainWindow)
        glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
    else
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);
}

void initAudio(){
	estado.buffer = alutCreateBufferFromFile("wave.wav");
	alGenSources(1, &estado.source);
	alSourcei(estado.source, AL_BUFFER, estado.buffer);
	estado.tecla_som = AL_FALSE;
}

/////////////////////////////////////
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	alutInit(&argc, argv);
	glutInitWindowPosition(10, 10);
	glutInitWindowSize(800+GAP*3, 400+GAP*2);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	if ((estado.mainWindow=glutCreateWindow("Labirinto")) == GL_FALSE)
	    exit(1);

    imprime_ajuda();
  
	
	
  // Registar callbacks do GLUT da janela principal
	init();
	glutReshapeFunc(reshapeMainWindow);
	glutDisplayFunc(displayMainWindow);

	glutTimerFunc(estado.timer,Timer,0);
	glutKeyboardFunc(Key);
	glutSpecialFunc(SpecialKey);
	glutSpecialUpFunc(SpecialKeyUp);

	// criar a sub window topSubwindow
	estado.topSubwindow=glutCreateSubWindow(estado.mainWindow, GAP, GAP, 400, 400);
	init();
	setLight();
	setMaterial();
    createTextures(modelo.texID[JANELA_TOP]);
	createDisplayLists(JANELA_TOP);

    mdlviewer_init("homer.mdl", modelo.homer[JANELA_TOP] );

	glutReshapeFunc(redisplayTopSubwindow);
	glutDisplayFunc(displayTopSubwindow);

	glutTimerFunc(estado.timer,Timer,0);
	glutKeyboardFunc(Key);
	glutSpecialFunc(SpecialKey);
	glutSpecialUpFunc(SpecialKeyUp);

	
	// criar a sub window navigateSubwindow
	estado.navigateSubwindow=glutCreateSubWindow(estado.mainWindow, 400+GAP, GAP, 400, 800);
	init();
	alutInit(&argc, argv);
	initAudio();
	setLight();
	setMaterial();
	
	createTextures(modelo.texID[JANELA_NAVIGATE]);
	createDisplayLists(JANELA_NAVIGATE);
	mdlviewer_init( "homer.mdl", modelo.homer[JANELA_NAVIGATE] );

	glutReshapeFunc(reshapeNavigateSubwindow);
	glutDisplayFunc(displayNavigateSubwindow);
	glutMouseFunc(mouseNavigateSubwindow);

	glutTimerFunc(estado.timer,Timer,0);
	glutKeyboardFunc(Key);
	glutSpecialFunc(SpecialKey);
	glutSpecialUpFunc(SpecialKeyUp);
	
	
	glutMainLoop();
	return 0;

}

