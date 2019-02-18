#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define PI 3.141592653589
#define TWO_PI 2 * PI
#define MAX_NUM_OF_POINTS 1000
#define MIN_NUM_OF_POINTS 3

/* Struktura tacke. Tacka je opisana sa x i y koordinatom i sa uglom u odnosu na pravu AP0, kao i sa flagovima za pravu oslonca i informacija o sracunatosti ugla */
struct coordinates
{
    double x, y;
    double angle_A;
    int oslonac;
    int calculated_angle;
};

/* Merenje vremena se obavlja pomocu CLOCK MONOTONIC-a - zbog najvece preciznosti */
static double CurrentTime(void)
{
    double time = 0;

    struct timespec t1;
    double test;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    test = ((t1.tv_sec) * 1000000000.0) + (t1.tv_nsec);
    return test;
}

/* Racunanje ugla izmedju zeljene tacke i prave P0A mnogougla */
static double get_angle(struct coordinates* coord, struct coordinates A, int num)
{
    /* Ne racunati ukoliko je sracunat vec */
    if (coord[num].calculated_angle == 0)
    {
        struct coordinates p0, p1, c;
        int i;
        double p0c, p1c, p0p1;

        p0 = coord[0];
        c = A;
        p1 = coord[num];

        p0c = sqrt ((pow(c.x - p0.x, 2) + pow(c.y - p0.y, 2)));
        p1c = sqrt((pow(c.x - p1.x, 2) + pow(c.y - p1.y, 2)));
        p0p1 = sqrt((pow(p1.x - p0.x, 2) + pow(p1.y - p0.y, 2)));

        /* Kosinusna teorema */
        coord[num].angle_A = acos((p1c * p1c + p0c * p0c - p0p1 * p0p1)/(2 * p1c * p0c));
        coord[num].calculated_angle = 1;
    }
    
    return coord[num].angle_A; 
}


/* Funkcija provere znaka uglova. Provera se vrsi utvrdjivanjem da li su obe tacke sa iste strane prave*/
/* t1 i t2 odredjuju pravu u odnosu na koju se odredjuje da li su tacke t3 i t4 sa iste strane */
/* Povratna vednost 1 ukoliko su sa iste strane, 0 ukoliko nisu */
static int check_sign(struct coordinates t1, struct coordinates t2, struct coordinates t3, struct coordinates t4)
{
    double A;
    double B;

    A = (t3.y - t1.y) * (t2.x - t1.x) - (t2.y - t1.y) * (t3.x - t1.x);
    B = (t4.y - t1.y) * (t2.x - t1.x) - (t2.y - t1.y) * (t4.x - t1.x);

    return (roundf(A * B) >= 0);
}

/* Binarna pretraga intervala gde se znak razlikuje*/
static void binary_search_interval(struct coordinates* coord, struct coordinates A, int* i, int* j, int num)
{
    int start = 0;
    int end = num-1;

    int found = 0;
    while(!found)
    {
        int tmp_i = (end + start) / 2;
        int ret = check_sign(coord[0], A, coord[1], coord[tmp_i]);
        if (ret == 0)
        {
		    end = tmp_i;
		    if (end == start)
		    {
            	found = 1;
            	*i = tmp_i-1;
             	*j = tmp_i;
		    }
        }
        else
        {
            start = tmp_i + 1;
        }
    }
}

/* Binarna pretraga maksimalnog ugla u odredjenom intervalu u odnosu na P0A */
static void binary_search_max(struct coordinates* coord, struct coordinates A, int i, int j)
{
    int start = i;
    int end = j;
    int found = 0;
    while(!found)
    {
        int tmp_i = (end + start) / 2;
        int tmp_j = tmp_i + 1;
        if (get_angle(coord, A, tmp_i) > get_angle(coord, A, tmp_j))
        {
            if (tmp_i == start)
            {
                found = 1;
                coord[tmp_i].oslonac = 1;
            }
            else
            {
                end = tmp_i;
            }
        }
        else if (get_angle(coord, A, tmp_i) < get_angle(coord, A, tmp_j))
        {
            if (tmp_j == end)
            {
                found = 1;
                coord[tmp_j].oslonac = 1;
            }
            else
            {
                start = tmp_j;
            }
        }
        else
        {
            found = 1;
            coord[tmp_j].oslonac = 1;
            coord[tmp_i].oslonac = 1;
        }

    }
}

/* Funkcija koja vrsi pronalazak pravih oslonaca za zadati mnogougao i tacku iz koje se trazi prava oslonca*/
static int processPolygon(struct coordinates* coord, struct coordinates A, int num)
{
    int start = 0;
    int end = num - 1;

    int start_case = check_sign(coord[0], A, coord[1], coord[num-1]);

    switch(start_case)
    {
        case 1:
        {
        	printf("istog znaka\n");
            /* Svi uglovi su istog znaka. AP0 je jedna prava oslonca, binarna pretraga druge po maks uglu*/
            coord[0].oslonac = 1;
            /* Provera da li i susedne tacke pripadaju mozda pravoj oslonca */
		    if(get_angle(coord, A, 0) == get_angle(coord, A, 1))
			{
				coord[1].oslonac = 1;
			}
			else if(get_angle(coord, A, 0) == get_angle(coord, A, num-1))
			{
				coord[num-1].oslonac = 1;
			}
            binary_search_max(coord, A, 1, num-1);
        }
        break;
        default:
        {
        	printf("suprotnog znaka su\n");
            /* nisu svi uglovi istog znaka. */
            int i, j;
            /* Pretraga intervala, pronalazak dve uzastopne tacke suprotnog znaka*/
            binary_search_interval(coord, A, &i, &j, num);
            /* Binarna pretraga max ugla u oba podintervala */
            binary_search_max(coord, A, 0, i);
            binary_search_max(coord, A, j, num-1);
        }
    }
}

/* Funkcija koja kreira mnogouglove sa zadatim brojem temena */
static void cratePolygon(struct coordinates* coord, double radius, int N)
{
  int i;
  double a;
  double angle = (TWO_PI) / N;
  double x = 0;
  double y = 0;

  /* Prva tacka sa najmanjom y koordinatom */
  a = TWO_PI - angle;
  while(a >= (TWO_PI * 3 / 4))
  {
  	a -= angle;
  }

  for (i = 0; i < N; i++) 
  {
    double sx = x + cos(a) * radius;
    double sy = y + sin(a) * radius;
    coord[i].x = sx;
    coord[i].y = sy;
    a += angle;
    if (a >= (TWO_PI))
    {
    	a = 0;
    }
  }
}

int main(int argc, char* argv[])
{

	printf(MAG "------------------------------------------------------------------------\n" RESET);
	printf(MAG "---- KONSTRUKCIJA PRAVIH OSLONACA IZ TACKE VAN KONVEKSNOG MNOGUGLA -----\n" RESET);
	printf(MAG "------------------------------------------------------------------------\n" RESET);
	printf("\n");

    int N;                                                                         /* Broj temena konveksnog mnogougla */
    int i;
    int radius;
    struct coordinates* coord;
    struct coordinates A;
	double last, delta, current;

 	N = 6;
 	radius = N;
 	printf(CYN "------------------------------------------------------------------------\n" RESET);
    printf(CYN "1) MNOGOUGAO SA %d TACAKA\n" RESET, N);
    printf("\n");

    /* Alokacija niza tacaka */
    coord = malloc(sizeof(struct coordinates) * N);
    memset(coord, 0, sizeof(struct coordinates) * N);

    /* Kreiranje mnogoougla*/
    cratePolygon(coord, radius, N);

    printf("Kordinate mnogougla:\n");
    for (i = 0; i < N; i++)
    {
        printf("\t- P%d = (%lf, %lf)\n", i, coord[i].x, coord[i].y);
    }
    printf("\n");

    A.x = 0;
    A.y = radius * 100;
    printf("1.1 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);

    /* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
    printf("\n" RESET);

    /* Resetovanje strukture za naredni podslucaj */
    for (i = 0; i < N; i++)
    {
	    coord[i].oslonac = 0;
		coord[i].calculated_angle = 0;
    }

    A.x = 4;
    A.y = coord[0].y;
    printf("1.2 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);
   
	/* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
    printf("\n" RESET);

	/* Resetovanje strukture za naredni podslucaj */
     for (i = 0; i < N; i++)
    {
	    coord[i].oslonac = 0;
		coord[i].calculated_angle = 0;
    }


    A.x = 4;
    A.y = 5;
	printf("1.3 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);
   
    /* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
 	printf("\n" RESET);
    
    free(coord);




 	N = 10;
 	radius = N;
 	printf(CYN "------------------------------------------------------------------------\n" RESET);
    printf(CYN "2) MNOGOUGAO SA %d TACAKA\n" RESET, N);
    printf("\n");

	/* Alokacija niza tacaka */
    coord = malloc(sizeof(struct coordinates) * N);
    memset(coord, 0, sizeof(struct coordinates) * N);

    /* Kreiranje mnogoougla*/
    cratePolygon(coord, radius, N);

    printf("Kordinate mnogougla:\n");
    for (i = 0; i < N; i++)
    {
        printf("\t- P%d = (%lf, %lf)\n", i, coord[i].x, coord[i].y);
    }
    printf("\n");

    A.x = 0;
    A.y = radius * 100;
    printf("2.1 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);

    /* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
    printf("\n" RESET);

	/* Resetovanje strukture za naredni podslucaj */
    for (i = 0; i < N; i++)
    {
	    coord[i].oslonac = 0;
		coord[i].calculated_angle = 0;
    }

    A.x = 4;
    A.y = coord[0].y;
    printf("2.2 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);
   
    /* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
    printf("\n" RESET);

    /* Resetovanje strukture za naredni podslucaj */
    for (i = 0; i < N; i++)
    {
	    coord[i].oslonac = 0;
		coord[i].calculated_angle = 0;
    }


    A.x = 10;
    A.y = 6;
	printf("2.3 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);
   
    /* Procesuiranje algoritma */   
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }

    for (i = 0; i < N; i++)
    {
        coord[i].oslonac = 0;
    }
 	printf("\n" RESET);
    free(coord);

 	N = 1000;
 	radius = N;
 	printf(CYN "------------------------------------------------------------------------\n" RESET);
    printf(CYN "3) MNOGOUGAO SA %d TACAKA\n" RESET, N);
    printf("\n");

	/* Alokacija niza tacaka */
    coord = malloc(sizeof(struct coordinates) * N);
    memset(coord, 0, sizeof(struct coordinates) * N);

    /* Kreiranje mnogoougla*/
    cratePolygon(coord, radius, N);

    printf("Kordinate mnogougla: (Pogledati fajl koordinate.txt)\n");
    // for (i = 0; i < N; i++)
    // {
    //     printf("\t- P%d = (%lf, %lf)\n", i, coord[i].x, coord[i].y);
    // }
    // printf("\n");

    A.x = 0;
    A.y = radius * 10000;
    printf("3.1 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);

    /* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
    printf("\n" RESET);

    /* Resetovanje strukture za naredni podslucaj */
    for (i = 0; i < N; i++)
    {
	    coord[i].oslonac = 0;
		coord[i].calculated_angle = 0;
    }

    A.x = 1005;
    A.y = coord[0].y;
    printf("3.2 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);
    
    /* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
    printf("\n" RESET);

    /* Resetovanje strukture za naredni podslucaj */
    for (i = 0; i < N; i++)
    {
	    coord[i].oslonac = 0;
		coord[i].calculated_angle = 0;
    }


    A.x = 1300;
    A.y = 1300;
	printf("3.3 Kordinata tacke iz koje se trazi oslonac: A = (%lf, %lf)\n", A.x, A.y);

    /* Procesuiranje algoritma */
    last = CurrentTime();
    processPolygon(coord, A, N);
    current = CurrentTime();
    delta = current - last;
    printf(GRN "Vreme potrebno za procesuiranje algoritma = %f ms \n" RESET, (double)delta/1000000.0);

    printf(BLU "Nadjene prave oslonca su :\n");
    for (i = 0; i < N; i++)
    {
        if (coord[i].oslonac == 1)
        {
            printf("P%d sa koordinatama (%lf, %lf)\n", i, coord[i].x, coord[i].y);
        }
    }
 	printf("\n" RESET);

    free(coord);

    return 0;
}

