//--------------------------------------------------------------------------//
// xmtnimage36.cc                                                           //
// Latest revision: 11-05-2004                                              //
// Copyright (C) 2004 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// fuzzy clustering                                                         //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

double *clusterdata;
double results[FUZZY_ITEMS];
char fuzzylabel[FUZZY_ITEMS][256];
inline double d(int i, int j);

//// Globals for partition

//-------------------------------------------------------------------------//
// partition                                                               //
//-------------------------------------------------------------------------//
void partition(void)
{
    static listinfo *fuzzyli;
    static Widget fuzzyform;
    int bpp,ino,i,j,k,count,x1,y1,x2,y2,v;
    double *pixels;
    double answers[FUZZY_ITEMS];
    for(k=0; k<g.image_count; k++) z[k].hit=0;

    fuzzyli = new listinfo;
    x1 = g.selected_ulx;
    y1 = g.selected_uly;
    x2 = g.selected_lrx;
    y2 = g.selected_lry;
    count = (y2-y1)*(x2-x1);
    if(!count) return;

    pixels = new double[count];
    fuzzyform = open_fuzzy_list(fuzzyli);
    k=0;
    for(j=y1;j<y2;j++)
    for(i=x1;i<x2;i++)
         pixels[k++] = readpixelonimage(i,j,bpp,ino);
    cluster(pixels, count, 2, 150, answers, fuzzyli, 1);
    update_fuzzy_list(fuzzyli);
    g.inmenu++;
    for(j=y1;j<y2;j++)
       for(i=y1;i<y2;i++)
       {   v = readpixelonimage(i,j,bpp,ino);
           if(v > answers[2]) setpixel(i,j,0,SET); 
           else setpixel(i,j,g.maxcolor,SET);
           if(ino>=0) z[ino].hit=1; 
       }
    g.inmenu--;
    delete[] pixels;
    for(k=0; k<g.image_count; k++) 
         if(z[k].hit){ rebuild_display(k); redraw(k);}
    redrawscreen();
    return;
}


//-------------------------------------------------------------------------//
// d - distance function for fuzzy clustering                              //
// Omit fabs() to get a sharp discontinuity between the classes.           //
// Different distance functions give drastically different coefficients.   //
// An exponent of 2 as used here makes the method a modified fuzzy k-means.//
//-------------------------------------------------------------------------//
inline double d(int i, int j)        // Distance function
{                                   
//   return (fabs(clusterdata[i] - clusterdata[j]));  
   return ((clusterdata[i] - clusterdata[j])*(clusterdata[i] - clusterdata[j]));
}


//-------------------------------------------------------------------------//
// cluster                                                                 //
// Classifies values in array `pixels' into c clusters and puts            //
// statistical data in array `results'.                                    //
// The input data are sorted and decimated using the `shrink' algorithm    //
//   to create N representative values. N can be increased for more accu-  //
//   rate results but algorithm is O(N^2).                                 //
// Parameters:                                                             //
//   pixels[]     =   input data (scaled 0..1)                             //
//   n            =   no. of input data points                             //
//   c            =   no. of clusters desired (ignored)                    //
// Calculates:                                                             //
//   results[0]   =   average of data points                               //
//   results[1]   =   no. of clusters actually found                       //
//   ...                                                                   //
// Also puts string array fuzzylabel of results in listinfo->info[].       //
//-------------------------------------------------------------------------//
void cluster(double *pixels, int n, int c, int N, double *answers, listinfo *li,
   int want_progress_bar)
{ 
   if(c > 16){ message ("Too many clusters"); return; }
   int clusters,i,k,n2=1+max(n,N),v;
   int labelcount=-1;
   Widget www, scrollbar;

   double *odata = new double[n2]; // Max. of N data points and C clusters
   for(k=0;k<n2;k++) odata[k]=0.0;
   clusterdata = new double[n2];   // Sorted data, shrunk or expanded to N data points
   if(clusterdata==NULL){ message(g.nomemory); delete[] odata; return; }
   for(k=0;k<n2;k++) clusterdata[k]=0.0;
   double *u_1d;
   double **u;                     // Membership function for each class
   double usquared,s1,s2,s3,s4,s5,s6,crossover,start,oneoverclusters,
          chisq, log2, prob, deviation, partition_coeff=0;
   double mean[8], mode[8], sumsq[8], entropy[8];
   int ncount[8];                  
   double *pixelcopy;
   int df;
   int *sorted;                   

   sorted = new int[n2];
   if(sorted==NULL){ message(g.nomemory); delete[] clusterdata; delete[] odata; return; }
   pixelcopy = new double[n];
   if(pixelcopy==NULL){ message(g.nomemory); delete[] sorted; delete[] clusterdata; delete[] odata; return; }

   u_1d = new double[N*c];
   if(u_1d==NULL){ message(g.nomemory); delete[] sorted; delete[] clusterdata; delete[] pixelcopy; delete[] odata; return; }
   for(k=0;k<N*c;k++) u_1d[k]=0.0;
   u = make_2d_alias(u_1d,N,c);
  
   if(want_progress_bar) progress_bar_open(www, scrollbar);

   for(k=0;k<FUZZY_ITEMS;k++) fuzzylabel[k][0]=0;
   if(want_progress_bar) progress_bar_update(scrollbar, 25);
   sort(pixels,sorted,n);
   if(want_progress_bar) progress_bar_update(scrollbar, 50);
   for(k=0;k<n;k++) odata[k] = pixels[sorted[k]];
   shrink(odata,n,clusterdata,N);
   if(want_progress_bar) progress_bar_update(scrollbar, 75);
   if(want_progress_bar) progress_bar_close(www);
   clusters = fuzzy_cluster(N,2,u,want_progress_bar);      


   //// Mean
   s1 = 0; for(k=0;k<n;k++) s1 += pixels[k]; s1/=n;
   results[++labelcount] = s1;
   strcpy(fuzzylabel[labelcount], "Mean of data                ");

   //// No. of clusters
   results[++labelcount] = (double)clusters;
   strcpy(fuzzylabel[labelcount], "No. of clusters             ");

   //// Crossover and crossover point
   crossover=0.0;
   start = sgn(u[0][0]-u[1][0]);
   for(i=0;i<N;i++) if(sgn(u[0][i]-u[1][i]) != start)
   {    crossover = clusterdata[i];  
        break;   
   }
   strcpy(fuzzylabel[++labelcount], "Demarcation point           ");
   results[labelcount] = crossover;

   if(clusters) oneoverclusters = 1.0/(double)clusters;
   else oneoverclusters = 0.0;
   for(i=0;i<n;i++) pixelcopy[i] = pixels[sorted[i]];
   smooth(pixelcopy,n,15);   
   smooth(pixelcopy,n,15);   
   smooth(pixelcopy,n,15);   
   log2 = log(2);

   for(v=0;v<clusters;v++)
   { 
        strcpy (fuzzylabel[++labelcount], "Cluster                     ");
        results[labelcount] = (double)v;
  
        //// Mean and total signal
        s2 = 0.0; s4 = 0.0; 
        for(i=0;i<N;i++) 
        {   s4 += clusterdata[i]*u[v][i];  
            s2 += u[v][i];
            partition_coeff += u[v][i]*u[v][i]/N;
        }
        sprintf(fuzzylabel[++labelcount], "  Weighted mean signal      ");
        if(s2!=0.0) mean[v] = s4/s2;
        else mean[v] = 0.0;
        results[labelcount] = mean[v];

        sprintf(fuzzylabel[++labelcount], "  Total signal in cluster %d ",v);
        results[labelcount] = mean[v]*n;       

        //// Maximum (mode) of the distribution.
        s2 = 0.0; s3 = 0.0;
        for(i=0;i<N;i++) 
        {    usquared =  u[v][i]*u[v][i];
             s2 += usquared * clusterdata[i];
             s3 += usquared;
        }
        if(s3!=0.0) mode[v] = s2/s3; else mode[v]=0;
        sprintf(fuzzylabel[++labelcount], "  Centroid (mode) density   ");
        results[labelcount] = mode[v];

        //// Variance, count, & SD 
        ncount[v] = 0;
        sumsq[v] = 0; 
        for(i=0;i<N;i++) 
           if(u[v][i] > oneoverclusters) 
           {    ncount[v]++;
                deviation = fabs(clusterdata[i] - mean[v]);
                sumsq[v] += deviation * deviation;           
           }          
        strcpy (fuzzylabel[++labelcount], "  Fraction of pixels:       ");
        results[labelcount] = (double)ncount[v]/(double)N;

        entropy[v] = 0;
        for(i=0;i<N;i++) 
            entropy[v] += u[v][i] * log(u[v][i]) / log2 +  
                          (1-u[v][i]) * log(1-u[v][i]) / log2; 
        entropy[v] = -entropy[v]/N;
   }

   sprintf(fuzzylabel[++labelcount], "  Normalized entropy        ");
   results[labelcount] = entropy[0];

   sprintf(fuzzylabel[++labelcount], " Net difference in density maxima ");
   results[labelcount] = mode[1] - mode[0];       

   //// chi square, and probability that u differs from TEST_VAL
   double f,f2;
   s5 = 0;
   for(i=0;i<n;i++) 
   {    f = pixels[i];
        f2 = mean[0];
        if(f2 != 0.0) s5 += (f-f2)*(f-f2)/f2;
   }
   chisq =  N*s5;
   strcpy(fuzzylabel[++labelcount], "Chi Squared                       ");
   results[labelcount] =chisq;
   df = N-2;
   prob = 1 - gammq(0.5*df, 0.5*chisq);
   strcpy(fuzzylabel[++labelcount], "Probability of 2 or more clusters ");
   results[labelcount] = prob;


   s6 = 0;
   for(i=0;i<N;i++) s6 += clusterdata[i];
   strcpy(fuzzylabel[++labelcount], "Total area                        ");
   results[labelcount] = s6;

   strcpy(fuzzylabel[++labelcount], "No. of pixels                     ");
   results[labelcount] = (double)n;

   strcpy(fuzzylabel[++labelcount], "Average density                   ");
   results[labelcount] = s6/n;
        
   strcpy(fuzzylabel[++labelcount], "Normalized partition coefficient  ");
   results[labelcount] = partition_coeff;

   strcpy(fuzzylabel[++labelcount], "Pixels in cluster 0               ");
   results[labelcount] = (double)n*ncount[0]/N;

   strcpy(fuzzylabel[++labelcount], "Pixels in cluster 1               ");
   results[labelcount] = (double)n*ncount[1]/N;

   if(labelcount>FUZZY_ITEMS) fprintf(stderr, "Error at %d %s\n",__LINE__,__FILE__);
   if(li == NULL || li->finished) 
        fprintf(stderr,"error in cluster %d\n",__LINE__);
   else for(k=0;k<FUZZY_ITEMS;k++) 
   {    strcpy(li->info[k], fuzzylabel[k]);
        answers[k] = results[k];
   }
   free(u);
   delete[] pixelcopy;
   delete[] u_1d;
   delete[] clusterdata;
   delete[] sorted;
   delete[] odata; 
   return;
}


//-------------------------------------------------------------------------//
// fuzzy_cluster                                                           //
// Classifies array `data' into c clusters, and puts the fuzzy coefficients//
//   for each class in array u.                                            //
// Based on fuzzy clustering algorithm (1,2), and equivalent to fuzzy      //
//   k-means (3,4).                                                        //
//  (1) Kaufman L and Rousseeuw PJ, Finding groups in data: an introduction//
//      to cluster analysis. New York: Wiley-Interscience, 1990.           //
//  (2) J.C. Bezdek, Pattern recognition with fuzzy objective function     //
//      algorithms. New York: Plenum, 1981.                                //
//  (3) J.C. Bezdek, Cluster validity with fuzzy sets. J.Cybern.3,58(1974).//
//  (4) Dunn, J. C., A fuzzy relative of the ISODATA process and its use   //
//      in detecting compact well-separated clusters. J.Cybern.3,32 (1974).//
//-------------------------------------------------------------------------//
int fuzzy_cluster(int n, int c, double**u, int want_progress_bar)  
{
   if(n>512) { message("Data set too large in fuzzy cluster"); return 0; }
   Widget www, scrollbar;
   printstatus(CALCULATING);
   const int ITERATIONS=20;   // Max. no. of iterations
   const double HI = 0.9;     // Initial estimate of which group it is in

   array<double> a(n+1,c+1);      // Fuzzy partition function
   array<double> oldu(n+1,c+1);   // Previous values, to test for convergence
   double A[512];             // Lagrange multipliers (psi)
   int V[512];                // Hard category
   double ave;
   int v,w,h,i,j,k,l,done=0;
   double s1=0,s2=0,s3=0,s4=0,s5=0,s6=0,s7=0,s8=0,s11=0,s12=0;
   double r1,r2;
   double LO = (1-HI)/(c-1);
   if(want_progress_bar) progress_bar_open(www, scrollbar);

   for(i=0;i<n;i++) s1 += clusterdata[i];
   ave = s1 / n;
   for(i=0;i<n;i++)
   {   if(clusterdata[i] < ave) u[0][i] = HI; else u[0][i] = LO;
       u[1][i] = 1 - u[0][i];
   }
     
   for(l=0;l<ITERATIONS;l++)
   { 
       if(want_progress_bar) progress_bar_update(scrollbar, l*100/(l+1));
       if(keyhit()) if (getcharacter()==27)break;
       if(g.getout) break;
       for(i=0;i<n;i++)
       {   if(g.getout) break;
           for(v=0;v<c;v++)
           {   s1=0;s2=0;s3=0;s4=0;s5=0;s6=0;s7=0;s8=0;s11=0;s12=0;
               for(j=0;j<i;j++) 
               {    r1 = u[v][j] * u[v][j];
                    r2 = r1 * d(i,j);
                    s1 += r2; 
                    s3 += r1; 
                    for(h=0;h<i;h++) s5 += r2 * u[v][h]*u[v][h];
                    for(h=i;h<n;h++) s6 += r2 * u[v][h]*u[v][h];
               }
               for(j=i;j<n;j++) 
               {    r1 = u[v][j] * u[v][j];
                    r2 = r1 * d(i,j);
                    s2 += r2; 
                    s4 += r1; 
                    for(h=0;h<i;h++) s7 += r2 * u[v][h]*u[v][h];
                    for(h=i;h<n;h++) s8 += r2 * u[v][h]*u[v][h];
               }
               if(s3+s4!=0) 
                  a.p[v][i] = (2*(s1+s2) - (s5+s6+s7+s8))/(s3+s4);
               else
                  a.p[v][i] = 1e6;
           }
           for(v=0;v<c;v++)
           {   s11 = 0;
               for(w=0;w<c;w++) if(a.p[w][i]!=0) s11 += 1/a.p[w][i];
               if(s11 && a.p[v][i])
                   A[v] = 1/a.p[v][i] / s11;
               else
                   A[v] = 1e6;
               if(A[v]<=0) V[v] = 1; else V[v] = 2;
           }
           for(v=0;v<c;v++)
           {   if(V[v] == 1) u[v][i]=0;
               if(V[v] == 2)
               {   s12 = 0;                
                   for(w=0;w<c;w++) if(V[w]==2 && a.p[w][i]!=0) s12 += 1/a.p[w][i];
                   if(s12 && a.p[v][i])
                       u[v][i] = 1/a.p[v][i] / s12;
                   else
                       u[v][i] = 1e6;
               }
           }
       }   

       done=1;
       for(k=0;k<c;k++)
       for(j=0;j<n;j++)
       {     if(fabs(oldu.p[k][j] - u[k][j]) > 0.001) done=0;
             oldu.p[k][j] = u[k][j];
       }
       if(done) break;
   }

   printstatus(DENSITOMETRY);
   if(want_progress_bar) progress_bar_close(www);
   if(l>=ITERATIONS-1) return 0; 
   return c;
}


//-------------------------------------------------------------------------//
// shrink - shrinks or expands a data set from m to n data points.         //
// Puts result in array x2.                                                //
//-------------------------------------------------------------------------//
void shrink(double *x, int m, double *x2, int n)
{
  int ii,j,k;
  double factor,inv_factor,test,frac,rem;
  if(m==0 || n==0) return;
  for(k=0;k<n;k++) x2[k]=0;
  k=0; j=0;
  factor = (double)m/(double)n;
  inv_factor = 1/factor;
  test=factor;
  if(m>n)             // shrink to n data points
  {    for(j=0;j<m;j++)
       {    if((double)(j+1) >= test)      
            {     frac = (test-(double)j)*x[j];
                  rem = x[j] - frac;
                  x2[k] += frac;
                  x2[++k] += rem;
                  test += factor;
            }else 
                  x2[k] += x[j];             
       }
       for(k=0;k<n;k++) x2[k] *= inv_factor;
  }else               // expand to n data points
  {  
       for(k=0;k<n;k++)
       {    ii = int(test+factor);
            if(int(test) != ii) 
            {     frac = ((double)ii-test)*x[j++];
                  rem = ((test+factor) - (double)ii)*x[j];
                  x2[k] = inv_factor*(frac+rem);
            }else 
                  x2[k] += x[j];             
            test += factor;
       }
  } 
}


//-------------------------------------------------------------------------//
// open_fuzzy_list                                                         //
// Parameter is a listinfo* that is already allocated but empty.           //
//-------------------------------------------------------------------------//
Widget open_fuzzy_list(listinfo *l)
{
   const int MAXITEMS=100;
   int k;
   Widget form;
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return 0; } 
   if(ci<0){ message("Please select an image"); return 0; }
  
   //-------------Put information in list box-------------------------//

   static char **info;                                    
   info = new char*[MAXITEMS];  // max. MAXITEMS lines - Change if you add more items
   for(k=0; k<FUZZY_ITEMS; k++){ info[k] = new char[256]; info[k][0]=0; }
   if(FUZZY_ITEMS>=MAXITEMS) fprintf(stderr, "Error: too many list items\n");
   static char *fuzzylisttitle;
   fuzzylisttitle = new char[256];
   strcpy(fuzzylisttitle, "Fuzzy partitioning results");

   l->title = fuzzylisttitle;
   l->info  = info;
   l->count = FUZZY_ITEMS;
   l->itemstoshow = 20;
   l->firstitem   = 0;
   l->wantsort    = 0;
   l->wantsave    = 1;
   l->helptopic   = 0;
   l->allowedit   = 0;
   l->selection   = &g.crud;
   l->width       = 0;
   l->transient   = 1;         // 0=user-positionable window 1=stays on top
   l->wantfunction = 0;        // use f1 to perform an action when item is selected
   l->autoupdate   = 0;        // use f2 to update list   
   l->clearbutton  = 0;
   l->highest_erased = 0;
   l->f1 = null;               // action to perform when item is selected
   l->f2 = null;               // how to update list 
   l->f3 = null;               // what to delete when done
   l->f4 = delete_list;
   l->f5 = null;
   l->f6 = make_fuzzy_list_unbusy;
   l->browser  = 0;
   l->finished = 0;
   l->wc       = 0;
   l->listnumber  = 2;
   fuzzy_list_fill(l);   
   form = list(l);   
   g.getout=0;
   return form;
}


//--------------------------------------------------------------------------//
// make_fuzzy_list_unbusy                                                   //
//--------------------------------------------------------------------------//
void make_fuzzy_list_unbusy(listinfo *l)
{
  l->busy = 0;
}


//--------------------------------------------------------------------------//
// update_fuzzy_list                                                        //
//--------------------------------------------------------------------------//
void update_fuzzy_list(listinfo *l)
{ 
   int k;
   if(!l || !l->title){ message("Error in fuzzy list"); return; }
   XmString *listitems=NULL;
   listitems = new XmString[100];
   fuzzy_list_fill(l);
   for(k=0; k<l->count; k++) listitems[k] = XmStringCreateSimple(l->info[k]);
   XmListReplaceItemsPosUnselected(l->widget, listitems, l->count, 1);
   for(k=0; k<l->count; k++) XmStringFree(listitems[k]);
   delete[] listitems;
}


//--------------------------------------------------------------------------//
// fuzzy_list_fill                                                          //
//--------------------------------------------------------------------------//
void fuzzy_list_fill(listinfo *l)
{
   int k;
   strcpy(l->title, "Area partitioning statistics");
   for(k=0; k<FUZZY_ITEMS; k++)
   {
        if(strlen(fuzzylabel[k]))
          sprintf(l->info[k],"%s  %g",fuzzylabel[k],results[k]);
       else strcpy(l->info[k]," ");
   }
}


//--------------------------------------------------------------------------//
//  gammln - gamma function using logs                                      //
//--------------------------------------------------------------------------//
double gammln(double x)
{
    double y,tmp,ser;
    const double cof[6] = { 76.18009172947146,  -86.50532032941677,
                            24.01409824083091,   -1.231739572450155,
                          0.1208650973866179e-2, -0.5395239384953e-5 };

    ////  x should already be > 0 but check just in case.
    if(x<0){ message("Bad parameters in gammln",ERROR); return 0.0; }
    int j;
    y = x;
    tmp = x + 5.5;
    tmp -= (x+0.5) * log(tmp);
    ser = 1.000000000190015;
    for(j=0;j<=5;j++){ ser += cof[j]/++y; }
    return -tmp + log(2.5066282746310005*ser/x);   
}


//--------------------------------------------------------------------------//
//  gammq - incomplete gamma function Q(a,x) = 1- P(a,x)                   //
//--------------------------------------------------------------------------//
double gammq(double a, double x)
{
    double gamser, gammcf, gln;
    if(x < 0.0 || a<= 0.0){ message("Bad parameters in gammq",ERROR); return 0.0; }
    if(x < a+1.0)
    {    gser(&gamser, a, x, &gln);
         return 1.0 - gamser;
    }else
    { 
         gcf(&gammcf, a, x, &gln);
         return gammcf;
    }
}


//--------------------------------------------------------------------------//
//  gser - incomplete gamma function P(a,x) evaluated by its series         //
//  representation as gamser. Also returns ln Gamma(a) as gln.              //
//--------------------------------------------------------------------------//
void gser(double *gamser, double a, double x, double *gln)
{
    int n;
    const int ITMAX = 100;
    const double EPS = 3.0e-7;
    double sum,del,ap;
 
    *gln = gammln(a);
    *gamser = 0.0;
    if(x<0.0){ message("x less than 0 in gser",ERROR); return; }   
    if(x==0.0) return;

    ap = a;
    del = sum = 1.0/a;
    for(n=0;n<ITMAX;n++)
    {   ap++;
        del *= x/ap;
        sum += del;
        if(fabs(del) < fabs(sum)*EPS)
        {    *gamser = sum*exp(-x+a*log(x)-(*gln));
             return;
        }
    }
    message("a too large in gser",ERROR); 
    return;
}


//--------------------------------------------------------------------------//
//  gcf - incomplete gamma function P(a,x) evaluated by its continued       //
//  fraction representation representation as gammcf.  Also returns         //
//  ln Gamma(a) as gln.                                                     //
//--------------------------------------------------------------------------//
void gcf(double *gammcf, double a, double x, double *gln)
{
   int i;
   const int ITMAX = 100;
   const double FPMIN = 1.0e-30;
   const double EPS = 3.0e-7;
   double an,b,c,d,del,h;
   *gln = gammln(a);
   b = x+1.0-a;
   c=1.0/FPMIN;   
   d = 1.0/b;
   h=d;
   for(i=1;i<=ITMAX;i++)
   {    an = -i*(i-a);
        b+=2.0;
        d=notzero(an*d+b);
        c=notzero(b+an/c);
        d=1.0/d;
        del=d*c;
        h *= del;
        if(fabs(del-1.0)<EPS) break;
   }
   if(i>=ITMAX) message("Too many iterations in gcf",ERROR); 
   *gammcf = exp(-x+a*log(x)- (*gln))*h;
}


//--------------------------------------------------------------------------//
//  notzero - prevent a number from being 0                                 //
//--------------------------------------------------------------------------//
double notzero(double a)
{
    const double FPMIN = 1.0e-100;
    if(fabs(a)<FPMIN) a = FPMIN;
    return a;
}
