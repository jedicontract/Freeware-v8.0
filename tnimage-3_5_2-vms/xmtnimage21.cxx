//--------------------------------------------------------------------------//
// xmtnimage21.cc                                                           //
// Evaluation of string expressions                                         //
// Latest revision: 03-21-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h" 

extern Globals     g;
extern Image      *z;
extern int         ci;
char *text = NULL;   // make a global so calculator.y can access it

//-------------------------------------------------------------------------//
// formula_error                                                           //
//-------------------------------------------------------------------------//
void formula_error(char *error_message, char *s)
{
    char tempstring[1024];
    sprintf(tempstring, "%s \n%s", error_message, s);
    tempstring[strlen(tempstring)-2] = 0; // chop off the 255
    message(tempstring, ERROR);
    g.getout=1; 
}

 
//-------------------------------------------------------------------------//
// eval - entry point into evaluating a string                             //
//-------------------------------------------------------------------------//
double eval(char* s, char *answer)
{   
   double dd=0;
   int datatype;
   char *text2;
   if(strlen(s)==0 || strlen(s)>1023) return 0;
   if(g.getout) return 0;
   text2 = new char[strlen(s)+8];
   swap_decimal_point(text2, s);  // this also copies s into text
   text = text2;
   if(strlen(text))
   {   strcat(text,"\255\0\0");
       yyparse();
       dd = read_variable((char*)"temp", answer, datatype);
   }
   delete[] text2;
   return dd;
}    


//-------------------------------------------------------------------------//
//  remove_leading_space                                                   //
//-------------------------------------------------------------------------//
void remove_leading_space(char *s1, char *s2)
{  
   int k, space=0;
   for(k=0;k<(int)strlen(s1);k++) if(s1[k]==' ') space=k+1; else break;
   strcpy(s2, s1+space);
}


//-------------------------------------------------------------------------//
//  remove_trailing_junk  junk being { } ; [space] or \n                   //
//-------------------------------------------------------------------------//
void remove_trailing_junk(char *s)
{
  if(strlen(s)) for(int k=strlen(s); k>=0; k--) if(strchr("{}; \n", s[k])) s[k]=0; else break;
}


//-------------------------------------------------------------------------//
//  remove_quotes - removes 1st & last char in string. Don't just reset    //
//  the ptr or it will crash when they free the memory.                    //
//-------------------------------------------------------------------------//
void remove_quotes(char *s)
{
  int len = (int)strlen(s);
  for(int k=0; k<len-2; k++) s[k]=s[k+1];
  s[len-2] = 0;
}


//-------------------------------------------------------------------------//
// set_value - s can be set(a=b);, set ( a="c" );, set(a=(func(a)));  etc. //
//-------------------------------------------------------------------------//
void set_value(char *s)
{
  char *s2 = strdup(s);
  remove_outer_parentheses(s2, '(', ')'); 
  strcat(s2,";");
  eval(s2, NULL); 
  free(s2);
}


//-------------------------------------------------------------------------//
// check_value                                                             //
//-------------------------------------------------------------------------//
void check_value(char *s)
{
  remove_outer_parentheses(s, '(', ')'); 
  strcat(s,";");
  evaluate(s, 1); // 1 = put in message box
}


//-------------------------------------------------------------------------//
// evaluate  - check a variable or expression                              //
//-------------------------------------------------------------------------//
void evaluate(char *string, int wantmessage)
{
  char **arg;
  arg = new char*[2];
  arg[0] = new char[1];
  arg[1] = new char[FILENAMELENGTH];
  strcpy(arg[1], string);
  evaluate(1, arg, wantmessage);   
  delete[] arg[1];
  delete[] arg[0];
  delete[] arg;
}

void evaluate(int noofargs, char **arg, int wantmessage)
{   
  int j,k, datatype;
  double dd=0;
  char *tempstring = new char[FILENAMELENGTH];
  char *tempstring2 = new char[FILENAMELENGTH];
  char *answer = new char[FILENAMELENGTH];
  if(noofargs<1) return;
  add_all_variables(ci);
  tempstring[0] = 0;
  strcpy(tempstring, arg[1]);
  for(k=2;k<=noofargs;k++)
  {   strcat(tempstring," "); 
      strcat(tempstring,arg[k]); 
  }
  answer[0] = 0;
  //// Try reading it as a variable name first - must strip off
  //// any parentheses, braces, etc. first
  j = 0;
  for(k=0;k<(int)strlen(tempstring);k++)
     if(isalnum(tempstring[k])){ tempstring2[j++]=tempstring[k]; tempstring2[j]=0; }

  dd = read_variable(tempstring2, answer, datatype);

  //// If not a variable, evaluate it as an expression.
  //// Use original string, since it might be sth. like sin(sdf+(erf(2)))
  if(datatype!=STRING && dd==0.0) 
      dd = eval(tempstring, answer);
  if(wantmessage)
  {   switch(datatype)
      {   case STRING: sprintf(tempstring, "string is %s", answer); break;
          case FLOAT:  // fall thru
          default: sprintf(tempstring, "value is %g", dd);
      }
      message(tempstring);
  }
  delete[] answer;
  delete[] tempstring;
  delete[] tempstring2;
}    


//-------------------------------------------------------------------------//
// evaluate_args - allows variable names in macro arguments                //
//-------------------------------------------------------------------------//
void evaluate_args(int noofargs, char **arg)
{   
  int k, datatype=NONE;
  double dd;
  char *answer = new char[FILENAMELENGTH];
  for(k=0; k<=noofargs; k++)  
  {   dd = read_variable(arg[k], answer, datatype);
      switch(datatype)
      {   case FLOAT: 
          case DOUBLE: sprintf(arg[k], "%g", dd); break;
          case STRING: if(strlen(answer)) strcpy(arg[k], answer); break;
      }
  }
  delete[] answer;
}


//-------------------------------------------------------------------------//
// swap_decimal_point                                                      //
//-------------------------------------------------------------------------//
void swap_decimal_point(char* text, char *s)
{
   int k;
   int len = (int)strlen(s);
   for(k=0; k<len; k++) 
   {
       if(s[k]==g.decimal_point && 
          between(k,1,len-1) &&
          isdigit(s[k-1]) && 
          isdigit(s[k+1])) text[k]='.';
       else text[k] = s[k];
   }
   text[len]=0;
}



//--------------------------------------------------------------------------//
//  erf                                                                     //
//--------------------------------------------------------------------------//
#ifndef HAVE_ERF
double erf(double x)
{
   double s1,s2,s3,s4,s5,y;
   s1 = fabs(x);
   s2 = 1 / (1 + .5 * s1);
   s3 = -.82215223 + s2 * .17087277;
   s3 = 1.48851587 + s2 * s3;
   s3 = -1.13520398 + s2 * s3;
   s3 = .27886807 + s2 * s3;            // Chebyshev fitting for
   s3 = -.18628806 + s2 * s3;           // estimation of
   s3 = s2 * (.09678418 + s2 * s3);     // complementary error function
   s3 = s2 * (1.00002368 + s2 * (.37049196 + s3));
   s5 = (-s1 * s1 - 1.26551223 + s3);
   if(s5 < -77) s5 = -77;
   s4 = s2 * exp(s5);
   if(x < 0) s4 = 2 - s4;
   y = s4;
   return y;
}
#endif

//-------------------------------------------------------------------------//
// mask                                                                    //
//-------------------------------------------------------------------------//
void mask(int noofargs, char **arg)
{
  int j,k;
  if(ci<0 && noofargs==0){ message("Select an image first"); return; }
  g.mask_ino1 = ci;
  if(g.mask_ino2 == 0) g.mask_ino2 = ci;
  static Dialog *dialog;

  if(noofargs==0)
  {
     dialog = new Dialog;
     if(dialog==NULL){ message(g.nomemory); return; }
     strcpy(dialog->title,"Mask  (Combine two images)");
     strcpy(dialog->radio[0][0],"Operation"); 
     strcpy(dialog->radio[0][1],"Mask (1&=2)");             
     strcpy(dialog->radio[0][2],"Inverse Mask (1&=~2)");             
     strcpy(dialog->radio[0][3],"Add (1+=2)");
     strcpy(dialog->radio[0][4],"Subtract (1-=2)");
     strcpy(dialog->radio[0][5],"Multiply (1*=2)");
     dialog->radiono[0] = 6;
     dialog->radioset[0] = g.mask_mode+1;

     strcpy(dialog->boxes[0],"Data");
     strcpy(dialog->boxes[1],"Image to change");
     strcpy(dialog->boxes[2],"Image for mask");
     strcpy(dialog->boxes[3],"X offset");
     strcpy(dialog->boxes[4],"Y offset");

     dialog->boxtype[0]=LABEL;
     dialog->boxtype[1]=INTCLICKBOX;
     dialog->boxtype[2]=INTCLICKBOX;
     dialog->boxtype[3]=INTCLICKBOX;
     dialog->boxtype[4]=INTCLICKBOX;

     dialog->boxmin[1]=1; dialog->boxmax[1]=g.image_count-1;
     dialog->boxmin[2]=1; dialog->boxmax[2]=g.image_count-1;
     dialog->boxmin[3]=-1000; dialog->boxmax[3]=1000;
     dialog->boxmin[4]=-1000; dialog->boxmax[4]=1000;
      
     for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
     itoa(g.mask_ino1, dialog->answer[1][0], 10);
     itoa(g.mask_ino2, dialog->answer[2][0], 10);
     itoa(g.mask_xoffset, dialog->answer[3][0], 10);
     itoa(g.mask_yoffset, dialog->answer[4][0], 10);   

     dialog->noofradios=1;
     dialog->noofboxes=5;
     dialog->helptopic=0;  
     dialog->want_changecicb = 0;
     dialog->f1 = maskcheck;
     dialog->f2 = null;
     dialog->f3 = null;
     dialog->f4 = maskfinish;
     dialog->f5 = null;
     dialog->f6 = null;
     dialog->f7 = null;
     dialog->f8 = null;
     dialog->f9 = null;
     dialog->width = 0;  // calculate automatically
     dialog->height = 0; // calculate automatically
     dialog->transient = 1;
     dialog->wantraise = 0;
     dialog->radiousexy = 0;
     dialog->boxusexy = 0;
     strcpy(dialog->path,".");
     dialog->message[0]=0;      
     dialog->busy = 0;
     dialogbox(dialog);
  }else
  {  if(noofargs>=1) g.mask_ino1 = image_number(arg[1]);
     if(g.getout){ g.getout=0; return; }
     if(noofargs>=2) g.mask_ino2 = image_number(arg[2]);
     if(g.getout){ g.getout=0; return; }
     if(noofargs>=3) g.mask_mode = atoi(arg[3]); 
     if(g.getout){ g.getout=0; return; }
     if(noofargs>=4) g.mask_xoffset = atoi(arg[4]); 
     if(g.getout){ g.getout=0; return; }
     if(noofargs>=5) g.mask_yoffset = atoi(arg[5]); 
     do_masking();
  }
}


//--------------------------------------------------------------------------//
//  maskfinish                                                              //
//--------------------------------------------------------------------------//
void maskfinish(dialoginfo *a)
{   
   a=a;
}


//--------------------------------------------------------------------------//
//  maskcheck                                                               //
//--------------------------------------------------------------------------//
void maskcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  radio=radio; box=box; boxbutton=boxbutton;
  if(radio != -2) return;   //// User clicked Ok or Enter

  g.mask_mode = a->radioset[0] - 1;
  g.mask_ino1 = atoi(a->answer[1][0]);
  g.mask_ino2 = atoi(a->answer[2][0]);
  g.mask_xoffset = atoi(a->answer[3][0]);
  g.mask_yoffset = atoi(a->answer[4][0]);
  if(g.mask_ino1 == g.mask_ino2) 
      if(message("Destination and mask images are the same.\nContinue?",
         YESNOQUESTION) !=YES) return;
  do_masking();
}

//--------------------------------------------------------------------------//
//  do_masking                                                              //
//--------------------------------------------------------------------------//
void do_masking(void)
{
  enum{ TMASK, TIMASK, TADDMASK, TSUBMASK, TMULMASK };
  int bpp, f,i,i2,i3,i4,j,rr1,gg1,bb1,rr2,gg2,bb2,value1,value2;
  int ino1 = g.mask_ino1;
  int ino2 = g.mask_ino2;
  if(z[ino1].bpp != z[ino2].bpp){message("Both images must be\nsame bits/pixel"); return; } 
  switch_palette(ino1);
  drawselectbox(OFF);
  bpp = z[ino1].bpp;  
  f = z[ino1].cf;  

  if(z[ino1].colortype==GRAY)
  {    for(j=0; j<z[ino1].ysize; j++)
       for(i=0,i2=0,i3=g.mask_xoffset,i4=g.mask_xoffset*g.off[bpp]; i<z[ino1].xsize; 
           i++, i2+=g.off[bpp], i3++, i4+=g.off[bpp])
       {   if(i>=z[ino2].xsize || j>=z[ino2].ysize) continue;
           if(i3>= z[ino1].xsize || j+g.mask_yoffset >= z[ino1].ysize) continue;
           if(i3<0 || j+g.mask_yoffset<0) continue;
           value1 = pixelat(z[ino1].image[f][j+g.mask_yoffset]+i4, bpp);
           value2 = pixelat(z[ino2].image[f][j]+i2, bpp);
           switch(g.mask_mode)
           {   case TMASK: if(!value2) value1=0; break;
               case TIMASK: if(value2) value1=0; break;
               case TADDMASK: value1 += value2; break;
               case TSUBMASK: value1 -= value2; break;
               case TMULMASK: value1 *= value2; break;
           }
           value1 = max(0, min(value1, (int)g.maxvalue[bpp]));
           putpixelbytes(z[ino1].image[f][j+g.mask_yoffset]+i4,value1,1,bpp);
       }
       z[ino1].hitgray = 0;   // Force remapping of gray levels
  }else
  {
       for(j=0; j<z[ino1].ysize; j++)
       for(i=0,i2=0,i3=g.mask_xoffset,i4=g.mask_xoffset*g.off[bpp]; i<z[ino1].xsize; 
           i++, i2+=g.off[bpp], i3++, i4+=g.off[bpp])
       {   if(i>=z[ino2].xsize || j>=z[ino2].ysize) continue;
           if(i3 >= z[ino1].xsize || j+g.mask_yoffset >= z[ino1].ysize) continue;
           if(i3<0 || j+g.mask_yoffset<0) continue;
           value1 = pixelat(z[ino1].image[f][j+g.mask_yoffset]+i4, bpp);
           value2 = pixelat(z[ino2].image[f][j]+i2, bpp);
           valuetoRGB(value1,rr1,gg1,bb1,bpp);
           valuetoRGB(value2,rr2,gg2,bb2,bpp);

           switch(g.mask_mode)
           {   case TMASK: if(!rr2) rr1=0; if(!gg2) gg1=0; if(!bb2) bb1=0; break;
               case TIMASK: if(rr2) rr1=0; if(gg2) gg1=0; if(bb2) bb1=0; break;
               case TADDMASK: rr1+=rr2; gg1+=gg2; bb1+=bb2; break;
               case TSUBMASK: rr1-=rr2; gg1-=gg2; bb1-=bb2; break;
               case TMULMASK: rr1*=rr2; gg1*=gg2; bb1*=bb2; break;
           }
           rr1 = max(0, min(rr1, g.maxred[bpp]));
           gg1 = max(0, min(gg1, g.maxgreen[bpp]));
           bb1 = max(0, min(bb1, g.maxblue[bpp]));
           value1 = RGBvalue(rr1,gg1,bb1,bpp);
           putpixelbytes(z[ino1].image[f][j+g.mask_yoffset]+i4,value1,1,bpp);
       }
  }
  z[ino1].touched=1;
  repair(ino1);
  return;
}



//-------------------------------------------------------------------------//
//  density - coordinates are relative to image.                           //
//-------------------------------------------------------------------------//
double density(int ino, int x1, int x2, int y1, int y2, int compensate, int invert)
{
  int x,y;
  double d = 0;
  for(y=y1-z[ino].ypos; y<=y2-z[ino].ypos; y++)
  for(x=x1-z[ino].xpos; x<=x2-z[ino].xpos; x++)
      d += pixeldensity(x,y,ci,compensate,invert);
  return d;
}


//-------------------------------------------------------------------------//
//  area                                                                   //
//-------------------------------------------------------------------------//
double area(int x1, int x2, int y1, int y2)
{
  printf("area %d  %d %d %d\n",x1,y1,x2,y2);
  return 0.0;
}

.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                