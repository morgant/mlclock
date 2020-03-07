/*
 *    Mlclock is "Macintsh like clock" .
 *    Written by Hideki Kimata.
 *    Send Email to hideki@hry.info.gifu-u.ac.jp.
 *    Access to http://www.hry.info.gifu-u.ac.jp/~hideki/index.html
 */

#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define VERSION "1.1"

#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAX_FORM 1000

#ifndef RCFILE
#define RCFILE ".mlclockrc"
#endif

#define FOCUS 0
#define CLICK 1
#define DEFAULTFONT "-*-*-medium-r-normal--14-*"
#define NEAR 10
#define FORM1 "(%a)%p%l:%M:%S"
#define FORM2 "(%a)%Y.%b.%d"
#define DEFAULTLANG "C"

int Mode;         /* timing of change form */
int Near;         /* the distance of activity */
int Head;         /* space of window head */
char *Form1;      /* display form of main */
char *Form2;      /* display form of second */
char *LocaleName; /* locale name */
char *FSName;     /* font set name */ 
char *RCFile;     /* path of config file */
int Color[2][3];  /* the color for font and background */
int Bold = False; /* software bold font */
 
void SetFont( char *localename , Display *dpy , char *fsname );
void readrc();
void getRGB( char *color , int *store );
void usage( char *name );

Display *dpy;
Window win;
XEvent eve;
XRectangle ink, logical;
GC gc;
XFontSet fs;
Colormap cmap;

struct timeval wait;

int main( int argc , char **argv ){
  char str[MAX_FORM] , str2[MAX_FORM] , *form = NULL;
  int screen , sw = False , lsec = 0 , uhead , width , height , i;
  struct tm *tmm;
  time_t tmt;

  Window root , child;
  int rootx , rooty , wx , wy;
  unsigned int key;
  XColor xcol;
  XSizeHints hint;
  int iro[2];
  
  wait.tv_usec = 200000;
  wait.tv_sec = 0;

  str[0] = '\0';
  i = 1;
  while( i < argc ){
    if ( !strcmp( "-display" , argv[i] )){
      i++;
      if ( argc >= argc )
	usage( argv[0]);
      strcpy( str , argv[i] );
      break;
    }
    usage(argv[0]);
  }
  
  dpy = XOpenDisplay(NULL);

  if ( dpy == NULL ){
    fprintf( stderr , "Can't open display.\n" );
    exit(0);
  }
  
  screen = DefaultScreen( dpy );
  gc = DefaultGC( dpy , screen );
  cmap = DefaultColormap( dpy , screen );
  
  readrc();
  
  if (XSupportsLocale() == False)
    fprintf(stderr,"X does not support the locale\n");
  SetFont( LocaleName , dpy , FSName );

  XSetForeground( dpy , gc , BlackPixel( dpy , screen ));
  setlocale(LC_TIME, LocaleName );
  
  time( &tmt );
  tmm = localtime( &tmt );
  tmm->tm_sec = 59;
  tmm->tm_min = 59;
  tmm->tm_hour = 23;
  tmm->tm_mday = 30;
  tmm->tm_yday = 365;

  strftime(str, MAX_FORM, Form1 , tmm);
  XmbTextExtents( fs , str , strlen(str) , &ink, &logical);
  width=logical.width;
  uhead = logical.y;
  height = logical.height;

  strftime(str, MAX_FORM, Form2 , tmm);
  XmbTextExtents( fs , str , strlen(str) , &ink, &logical);
  if ( width < logical.width )
    width = logical.width;
  if ( uhead < logical.y )
    uhead = logical.y;
  if ( height < logical.height )
    height = logical.height;

  xcol.flags = DoRed|DoGreen|DoBlue;
  for ( i = 0; i < 2; i ++ ){
    xcol.red =  Color[i][0];
    xcol.green = Color[i][1];
    xcol.blue = Color[i][2];
    if ( XAllocColor( dpy , cmap , &xcol ))
      iro[i] = xcol.pixel;
    else fprintf(stderr,"Can't allocate Color \n" );
  }
  
  win = XCreateSimpleWindow( dpy , DefaultRootWindow( dpy ) ,
			    0 , 0 , width + 3 , height + Head + 2 , 0 , 
			    iro[0] ,iro[1] ); 

  hint.max_width  = hint.min_width  = width + 3;
  hint.max_height = hint.min_height = height + Head + 2;
  hint.flags = PMinSize | PMaxSize;
  XSetNormalHints(dpy , win , &hint);
  
  XSelectInput( dpy , win , ButtonReleaseMask );
  XStoreName( dpy , win , "mlclock" );
  XSetForeground( dpy , gc , iro[0] );
  XSync( dpy , 0 ); 
  XMapWindow( dpy ,win );
  XSync( dpy , 0 ); 
  
  str2[0] = '\0';
  form = Form1;
  while(1){
    select( 0 , (fd_set *)0 , (fd_set *)0 ,  (fd_set *)0 , &wait);

    time( &tmt );
    tmm = localtime( &tmt );

    if ( Mode == FOCUS ){
      XQueryPointer( dpy , win , &root , &child , &rootx,&rooty , &wx,&wy,&key);
      if (( -Near < wx ) && ( wx < width + Near ) && 
	  ( -Near < wy ) && ( wy < height + Near + Head ))
	form = Form2;
      else
	form = Form1;    }
    else
      if ( Mode == CLICK ){
	if( XEventsQueued( dpy, QueuedAfterFlush ) ){
	  XNextEvent( dpy , &eve );
	  if ( eve.type == ButtonRelease ){
	    if ( !sw ){
	      sw = True;
	      lsec = tmm->tm_sec;
	      form = Form2;
	    }
	    else {
	      sw = False;
	      form = Form1;
	    }
	  }
	}
	
	if ( sw ){
	  if ( tmm->tm_sec < lsec ) 
	    tmm->tm_sec += 60;
	  if ( tmm->tm_sec > lsec + 2 ){
	    sw = False;
	    form = Form1;
	  }
	}
      }
    
    strftime(str, sizeof(str),form , tmm);
    if ( strcmp( str , str2 )){
      XClearWindow( dpy , win );
      XmbDrawString( dpy , win , fs , gc, 1 , - uhead + Head + 1 , str, strlen(str));
      if ( Bold )
	XmbDrawString( dpy , win , fs , gc, 2 , - uhead + Head + 1 , str, strlen(str));
      XSync( dpy , 0 );
      strcpy( str2 , str );
    }
  }

  return 0;
}

void SetFont( char *localename , Display *dpy , char *fsname )
{
  char **miss, *def;
  int nMiss;

  if (setlocale(LC_ALL, localename) == NULL)
    fprintf(stderr,"Can't set the locale\n");

  fs = XCreateFontSet( dpy , fsname , &miss, &nMiss, &def);
  if (fs == NULL)
    fprintf(stderr,"Can't get fontset.\n" );
}

void readrc(){
  int i , end , len , sw1 , cn = 0;
  char string[501] , work[201] , code[100] , data[100] , *ptr;
  char *fore , *back;
  struct _name {
    char name[10];
    int sw;
  };

  FILE *file;
  
  struct _name N[] ={
    { "FORM1" , 1 } ,
    { "FORM2" , 2 } ,
    { "NEAR" , 3 } ,
    { "EVENT" , 4 } ,
    { "LOCALE" , 5 } ,
    { "FONTSET" , 6 } ,
    { "HEAD" , 7 } ,
    { "FONTCOLOR" , 8 } ,
    { "BACKCOLOR" , 9 } ,
    { "BOLD" , 10 } ,
    { "" , 0 }};

  
  Mode = Near = Head = NONE;
  Form1 = Form2 = LocaleName = FSName = NULL;

  for ( i = 0; i < 3; i ++ ){
    Color[0][i] = 0;
    Color[1][i] = 0xffff;
  }
  
  RCFile = malloc ( sizeof( RCFILE ) + strlen( getenv("HOME")) + 3 );
  sprintf( RCFile , "%s/%s",getenv("HOME"),RCFILE );

  if ( ( file = fopen( RCFile , "r" )) == NULL ){
    fprintf( stderr , "Can't open \"%s\" file.\n" , RCFile );
    fprintf( stderr , "Now making rc file.\n" , RCFile );
    
    if ((file = fopen( RCFile , "a" )) == NULL )
      fprintf( stderr , "You can't make rc file in your own dirctory.\n" );
    else{
      fprintf( file , "FORM1	= \"(%%a)%%l:%%M:%%S%%p\"\n");
      fprintf( file , "FORM2	= \"(%%a)%%b-%%d\"\n");
      fprintf( file , "EVENT	= FOCUS	; FOCUS or CLICK\n");
      fprintf( file , "NEAR	= 10\n" );
      fprintf( file , "LOCALE	= \"C\"\n");
      fprintf( file , "FONTSET	= \"-*-*-medium-r-normal--12-*\"\n");
      fprintf( file , "HEAD	= 5\n");
      fprintf( file , "FONTCOLOR = \"blue\"\n" );
      fprintf( file , "BACKCOLOR = \"white\"\n" );
      fprintf( file , "BOLD = FALSE  ; TRUE or FALSE\n" );
      fclose ( file );
      fprintf( stderr , "Created %s file.\n" , RCFile );
    }
  }
  else
  while( fgets( string , 500 , file )){
    end = FALSE;
    i = 0;
    sw1 = False;
    while( string[i] != ';' && string[i] != '\0' && string[i] != '\n' ){
      if ( string[i] == '=' && sw1 == False ){
	string[i] = ' ';
	sw1 = True;
      }
      work[i] = string[i];
      i++;
    }
    work[i] = '\0';
    
    if ( sw1 == False )
      continue;
    
    code[0] = '\0';
    data[0] = '\0';
    
    sscanf( work , "%s %s", code , data);
    if ( code[0] == '\0' ) 
      continue;

    i = 0;
    while( True ){
      if ( N[i].name[0] == '\0' ){
	cn = 0;
	break;
      }
      if ( !strcmp( N[i].name , code )){
	cn = N[i].sw;
	break;
      }
      i ++;
    }
    
    switch( cn ){
    case 1:
      len = strlen( data );
      Form1 = malloc( len );
      strcpy( Form1 , &data[1] );
      Form1[len-2] = '\0';
      break;
    case 2:
      len = strlen( data );
      Form2 = malloc( len );
      strcpy( Form2 , &data[1] );
      Form2[len-2] = '\0';
      break;
    case 3:
      Near = atoi( data );
      break;
    case 4:
      if ( !strcmp( data , "FOCUS" )){
	Mode = FOCUS;
	break;
      }
      if ( !strcmp( data , "CLICK" )){
	Mode = CLICK;
	break;
      }
      break;
    case 5:
      len = strlen( data );
      LocaleName = malloc( len );
      strcpy( LocaleName , &data[1] );
      LocaleName[len-2] = '\0';
      break;
    case 6:
      len = strlen( data );
      FSName = malloc( len );
      strcpy( FSName , &data[1] );
      FSName[len-2] = '\0';
      break;
    case 7:
      Head = atoi( data );
      break;
    case 8:
      len = strlen( data );
      fore = malloc( len );
      strcpy( fore , &data[1] );
      fore[len-2] = '\0';
      getRGB( fore , Color[0] );
      break;
    case 9:
      len = strlen( data );
      back = malloc( len );
      strcpy( back , &data[1] );
      back[len-2] = '\0';
      getRGB( back , Color[1] );
      break;
    case 10:
      if ( !strcmp( data , "TRUE" )){
	Bold = True;
	break;
      }
      if ( !strcmp( data , "FALSE" )){
	Bold = False;
	break;
      }
      break;
    default:
      break;
    }
  }
  
  if ( Mode == NONE )
    Mode = FOCUS;
  if ( Near == NONE )
    Near = NEAR;
  if ( Head == NONE )
    Head = 0;
  if ( Form1 == NULL ){
    Form1 = malloc( strlen( FORM1 ) + 1);
    strcpy( Form1 , FORM1 );
  }
  if ( Form2 == NULL ){
    Form2 = malloc( strlen( FORM2 ) + 1);
    strcpy( Form2 , FORM2 );
  }
  if ( FSName == NULL ){
    FSName = malloc( strlen( DEFAULTFONT ) + 1);
    strcpy( FSName , DEFAULTFONT );
  }
  if ( LocaleName == NULL ){
    ptr = getenv( "LANG" );
    if ( ptr == NULL )
      ptr = DEFAULTLANG;
    LocaleName = malloc( strlen( ptr ) + 1);
    strcpy( LocaleName , ptr );
  }
}

void getRGB( char *color , int *store ){
  int i;
  char tmpcolor[3];
  XColor rgb , hard;
  
  if ( color[0] == '#' ){
    color ++;
    for ( i = 0; i < 3; i ++ ){
      strncpy( tmpcolor , color , 2 );
      sscanf( tmpcolor , "%x" , store );
      (*store) *= 256;
      color += 2;
    }
  }
  else {
    XLookupColor( dpy , cmap , color , &rgb , &hard );
    store[0] = hard.red;
    store[1] = hard.green;
    store[2] = hard.blue;
  }
}

void usage( char *name ){
  printf("%s: usage\n" , name );
  printf("  -display      display name\n\n" );
  printf(" Version %s\n" , VERSION );
  printf(" Written by Hideki Kimata\n");
  printf(" EMail hideki@hry.info.gifu-u.ac.jp\n" );
  printf(" Access to http://www.hry.info.gifu-u.ac.jp/~hideki/index.html\n" );
  exit(0);
}
