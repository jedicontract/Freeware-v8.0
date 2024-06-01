//--------------------------------------------------------------------------//
// xmtnimage45.cc                                                           //
// Latest revision: 10-12-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

//--------------------------------------------------------------------------//
// test1                                                                    //
//--------------------------------------------------------------------------//
void test1(void)
{
}

//--------------------------------------------------------------------------//
//  cset - set misc. params after clickbox.  Better than having dozens of   //
//  one-line callbacks or several types of function ptrs.                   //
//  Caller data are passed through c->ptr[4].                               //
//--------------------------------------------------------------------------//
void cset(clickboxinfo *c)
{
  int k, value=0;
  int *clientptr = (int*)c->client_data;
  if(clientptr != NULL) value = *clientptr;
  PlotData *pd; 
  switch(c->identifier)
  {  case 0: break;  // no callback needed
     case 1: g.draw_figure=BOX; 
             g.line.width = c->answer;
             break;
     case 2: g.draw_figure=CIRCLE; 
             g.diameter = c->answer;
             break;
     case 3: transparency_set_image(value, c->answer); break;
     case 4: changecolordepth_part2(ci, c->answer); break;
     case 5: dialogclickboxcb_part2(c); break;
     case 6: dialogdoubleclickboxcb_part2(c); break;
     case 7: break; // available
     case 8: apply_newpalette(); break;
     case 9: randompalette(c->answer);
             g.currentpalette = c->answer; 
             setpalette(g.palette); 
             break;
     case 10: remap_image_colormap_part2(c); break;
     case 11:
              pd = (PlotData*)c->client_data;
              for(k=0; k<pd->n; k++) pd->data[pd->focus][k] += c->answer;
              graph_adjust(c);
              break;
     case 12: pd = (PlotData*)c->client_data;
              for(k=0; k<pd->n; k++) pd->data[pd->focus][k] *= c->fanswer;
              graph_adjust(c);
              break;
     case 13: pd = (PlotData*)c->client_data;
              draw_graph(pd, 0, pd->n);
              break;
     case 14: break;
     case 15: g.draw_figure=CROSS; 
              g.cross_length = c->answer;
              break;
     case 16: g.draw_pattern = c->answer; break;
     
  }
}


//--------------------------------------------------------------------------//
// sanity_check                                                             //
// Add any checks for incorrect global values here                          //
//--------------------------------------------------------------------------//
void sanity_check(void)
{
   if(g.getout < 0){ fprintf(stderr, "getout error\n"); g.getout = 0; }
   if(g.waiting < 0){ fprintf(stderr, "waiting error\n"); g.waiting = 0; }
   if(g.draw_figure < 0){ fprintf(stderr, "draw_figure error\n"); g.draw_figure = 0; }
   if(g.state < 0){ fprintf(stderr, "state error\n"); g.state = 0; }
   if(g.busy < 0){ fprintf(stderr, "busy error\n"); g.busy = 0; }
   if(g.inmenu < 0){ fprintf(stderr,"Error: inmenu=%d\n",g.inmenu); g.inmenu = 0; }
   if(g.block < 0){ fprintf(stderr, "block error\n"); g.block = 0; }
}



//--------------------------------------------------------------------------//
// null - do nothing funcs for clickbox(), plot(), etc.                     //
//--------------------------------------------------------------------------//
void null(int a)
{ a=a; }
void null(int a[10])
{ a[0]=a[0]; }
void null(double a)
{ a=a; }
void null(void)
{}
void null(char *s)
{ s=s; }
void null(pcb_struct v)
{ v.x=v.x; }
void null(dialoginfo *a, int b, int c, int d)
{ a=a; b=b; c=c; d=d; }
void null(dialoginfo *a)
{ a=a; }
void null(listinfo *a)
{ a=a; }
void null(listinfo *l, int a)
{ a=a; l=l; }
void null(clickboxinfo *c)
{ c=c; }
void null(PromptStruct *ps)
{ ps=ps; }
void null(int k, clickboxinfo *c)
{ k=k; c=c;}
void null(void *p, int n1, int n2)
{   PlotData *pd = (PlotData *)p;
    pd->ymin=0;
    pd->ymax=0;
    n1=n1; n2=n2; 
}
void null(void *ptr, char *string)
{ ptr=ptr; string=string; }
void null(void *ptr, char *string, int answer)
{ ptr=ptr; string=string; answer=answer; }
void null(Widget w, void *ptr1, void *ptr2)
{ w=w; ptr1=ptr1; ptr2=ptr2; }


//--------------------------------------------------------------------------//
// print diagnostic information (for debugging)                             //
//--------------------------------------------------------------------------//
void print_diagnostic_information(void)
{ 
   printf("block= %d %d %d\n",g.block, g.busy, g.waiting);
   if(between(ci, 0, g.image_count-1))
   {    printf("bpp %d %d\n",z[ci].bpp,z[ci].colortype);
        printf("c %d %d %d  %d %d %d  %d %d %d  %d \n",z[ci].image[0][5][0],z[ci].image[0][5][1],z[ci].image[0][5][2],z[ci].image[0][5][3],z[ci].image[0][5][4],z[ci].image[0][5][5],z[ci].image[0][5][6],z[ci].image[0][5][7],z[ci].image[0][5][8],z[ci].image[0][5][9]);
        if(z[ci].backedup)
        printf("c %d %d %d  %d %d %d  %d %d %d  %d \n",z[ci].backup[0][5][0],z[ci].backup[0][5][1],z[ci].backup[0][5][2],z[ci].backup[0][5][3],z[ci].backup[0][5][4],z[ci].backup[0][5][5],z[ci].backup[0][5][6],z[ci].backup[0][5][7],z[ci].backup[0][5][8],z[ci].backup[0][5][9]);
        printf("c %d %d %d  %d %d %d  \n",z[ci].palette[0].red,z[ci].palette[0].green,z[ci].palette[0].blue,z[ci].palette[127].red,z[ci].palette[127].green,z[ci].palette[127].blue);
        printf("c %d %d %d  %d %d %d  \n",z[ci].spalette[0].red,z[ci].spalette[0].green,z[ci].spalette[0].blue,z[ci].spalette[127].red,z[ci].spalette[127].green,z[ci].spalette[127].blue);
   }
   char tempstring[1024] = "Testing";
   int answer;
   if((answer = message("Testing message", QUESTION)) == YES)
   if((answer = message("Testing message", QUESTION)) == YES)
   if((answer = message("Testing message", QUESTION)) == YES)
   if((answer = message("Testing message", QUESTION)) == YES)
   if((answer = message("Testing message", QUESTION)) == YES)
     printf("messages ok\n");
   message("Testing message", tempstring, PROMPT, 54, 54);
   printf("prompt returned %s\n",tempstring);
   message("Testing message", tempstring, PROMPT, 54, 54);
   printf("prompt returned %s\n",tempstring);
   message("Testing message", tempstring, PROMPT, 54, 54);
   printf("prompt returned %s\n",tempstring);
   message("Testing message", tempstring, PROMPT, 54, 54);
   printf("prompt returned %s\n",tempstring);
   message("Testing message", tempstring, PROMPT, 54, 54);
   printf("prompt returned %s\n",tempstring);
   message("Testing message", tempstring, PROMPT, 54, 54);
   printf("prompt returned %s\n",tempstring);
   if((answer = message("Testing y/n message", YESNOQUESTION)) == YES)
   if((answer = message("Testing y/n message", YESNOQUESTION)) == YES)
   if((answer = message("Testing y/n message", YESNOQUESTION)) == YES)
   if((answer = message("Testing y/n message", YESNOQUESTION)) == YES)
   if((answer = message("Testing y/n message", YESNOQUESTION)) == YES)
     printf("messages ok\n");
}
