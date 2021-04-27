/* Pract2  RAP 09/10    Javier Ayllon*/

#include <openmpi/mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h> 
#include <assert.h>   
#include <unistd.h>  

#define NIL (0) 
#define N 5 /*número máximo de procesos*/
#define IMAGEN "src/foto.dat"   
#define TAM_IMAGEN 400  
#define TIPO_FILTRO 3 /*0, sin filtro; 1, sepia; 2, blanco y negro; 3, invertir colores*/ 

/*Variables Globales */

XColor colorX;
Colormap mapacolor;
char cadenaColor[]="#000000";
Display *dpy;
Window w;
GC gc;

/*Funciones auxiliares */

/*Método que inicializa X11 para trabajar con ventanas*/
void initX() { //va a crear una ventana para que sea de 400x400, la llamamos y ya nos crea la ventana

      dpy = XOpenDisplay(NIL);
      assert(dpy);

      int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
      int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

      w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                     400, 400, 0, blackColor, blackColor); //aqui es donde se indica el 400x400
      XSelectInput(dpy, w, StructureNotifyMask);
      XMapWindow(dpy, w);
      gc = XCreateGC(dpy, w, 0, NIL);
      XSetForeground(dpy, gc, whiteColor);
      for(;;) {
            XEvent e;
            XNextEvent(dpy, &e);
            if (e.type == MapNotify)
                  break;
      }


      mapacolor = DefaultColormap(dpy, 0);

}

/*Método que dibuja un punto*/
void dibujaPunto(int x,int y, int r, int g, int b) {

        sprintf(cadenaColor,"#%.2X%.2X%.2X",r,g,b);
        XParseColor(dpy, mapacolor, cadenaColor, &colorX);
        XAllocColor(dpy, mapacolor, &colorX);
        XSetForeground(dpy, gc, colorX.pixel);
        XDrawPoint(dpy, w, gc,x,y);
        XFlush(dpy);

}

/*Método que recibe los puntos que se dibujarán posteriormente*/
void reciboPuntos(MPI_Comm commPadre){
      int bufferPuntos[5]; //buffer que almacena las coordenadas y el color que se va a pintar
      MPI_Status status; 
      
      /*Obtengo los diferentes puntos que forman la imagen*/
      for(int i = 0; i < (TAM_IMAGEN*TAM_IMAGEN); i++){

            /*Recibo los puntos*/
            MPI_Recv(&bufferPuntos, 5, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, commPadre, &status); 

            /*Llamamos al método que dibuja el punto*/
            dibujaPunto(bufferPuntos[0], bufferPuntos[1], bufferPuntos[2], bufferPuntos[3], bufferPuntos[4]); 
      }
}

/* Programa principal */
int main (int argc, char *argv[]) {

  int rank,size;
  MPI_Comm commPadre; //intercomunicador de los trabajadores 
  int tag;
  MPI_Status status;
  int bufferPuntos[5]; //buffer que almacena las coordenadas y el color que se va a pintar
  int errCodes[N]; //array de códigos de error al lanzar a los trabajadores

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_get_parent( &commPadre );

  /*Si es el maestro*/
  if ((commPadre==MPI_COMM_NULL) && (rank==0)){

	initX();

      /*Se lanzan los procesos trabajadores*/
      MPI_Comm_spawn("exec/pract2", MPI_ARGV_NULL, N, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &commPadre, errCodes); 
      /*Recibimos los puntos de los trabajadores y se dibujan*/
      reciboPuntos(commPadre); 
      printf("La imagen se ha dibujado.\nSe mostrará durante 4 segundos.\n"); 
      /*Segundos que vamos a mostrar la imagen*/
      sleep(4); 

      }

  else {
    /*  TRABAJADORES - RESTO RANKS */
    /*Manejador del archivo*/
    MPI_File fh; 

    /*Número de filas que lee cada trabajador*/
    int numFilasTrab = TAM_IMAGEN / N; 

    /*Tamaño del bloque que lee cada trabajador*/
    int tamBloqueTrab = numFilasTrab * TAM_IMAGEN * 3 * sizeof(unsigned char); 

    /*Inicio y fin de la lectura, para situarnos en la matriz donde empezamos y acabamos de leer*/
    int inicioLect = rank * numFilasTrab; 
    int finLect = inicioLect + numFilasTrab; 

    /*Abrimos la imagen con permisos de lectura*/
    MPI_File_open(MPI_COMM_WORLD, IMAGEN, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    /*Establecemos la vista para que solo accedamos a los datos correspondientes a mi rank (al bloque correspondiente)*/
    MPI_File_set_view(fh, rank * tamBloqueTrab, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, "native", MPI_INFO_NULL); 

    /*Variable que almacena el color de un punto de la matriz*/
    unsigned char colorPunto[3]; 

    /*Compruebo que soy el último rank */
    if(rank == N-1){
          finLect = TAM_IMAGEN; 
    }

    /*Recorremos los bloques para aplicar el filtro deseado*/
    int i, j, k; 

    for(i = inicioLect; i < finLect; i++){
          for(j = 0; j < TAM_IMAGEN; j++){

                MPI_File_read(fh, colorPunto, 3, MPI_UNSIGNED_CHAR, &status); 

                /*Coordenadas*/
                bufferPuntos[0] = j; 
                bufferPuntos[1] = i; 

                /*Le aplicamos el filtro que hayamos definido a la imagen*/
                  switch (TIPO_FILTRO){
                        case 0: /*sin filtro*/
                              bufferPuntos[2] = (int)colorPunto[0]; /*R*/
                              bufferPuntos[3] = (int)colorPunto[1]; /*G*/
                              bufferPuntos[4] = (int)colorPunto[2]; /*B*/
                              break;
                
                        case 1: /*sepia*/
                              bufferPuntos[2] = ((int)colorPunto[0] * 0.393) + ((int)colorPunto[1] * 0.769) + ((int)colorPunto[2] * 0.189); 
                              bufferPuntos[3] = ((int)colorPunto[0] * 0.349) + ((int)colorPunto[1] * 0.686) + ((int)colorPunto[2] * 0.168); 
                              bufferPuntos[4] = ((int)colorPunto[0] * 0.272) + ((int)colorPunto[1] * 0.534) + ((int)colorPunto[2] * 0.131); 
                              break;
                        
                        case 2: /*blanco y negro*/
                              bufferPuntos[2] = ((int)colorPunto[0] + (int)colorPunto[1] + (int)colorPunto[2]) / 3; 
                              bufferPuntos[3] = ((int)colorPunto[0] + (int)colorPunto[1] + (int)colorPunto[2]) / 3;
                              bufferPuntos[4] = ((int)colorPunto[0] + (int)colorPunto[1] + (int)colorPunto[2]) / 3;
                              break; 

                        case 3: /*invertir colores*/
                              bufferPuntos[2] = 255 - (int)colorPunto[0]; /*R*/
                              bufferPuntos[3] = 255 - (int)colorPunto[1]; /*G*/
                              bufferPuntos[4] = 255 - (int)colorPunto[2]; /*B*/ 
                              break;
                  }

                  /*Comprobamos que no sea un color incorrecto*/
                  for(k = 2; k < 5; k++){
                        if(bufferPuntos[k] > 255){
                              bufferPuntos[k] = 255; 
                        }
                  }
                  /*Enviamos la imagen ya con filtro */
                  MPI_Bsend(&bufferPuntos, 5, MPI_INT, 0, 1, commPadre); 
            }
      }

      MPI_File_close(&fh); 
    }

  MPI_Finalize();

  return EXIT_SUCCESS; 

}