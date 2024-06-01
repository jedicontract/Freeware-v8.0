//--------------------------------------------------------------------------//
// xmtnimage6.cc                                                            //
// Help system                                                              //
// Latest revision: 09-03-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//
 
#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;

 
//--------------------------------------------------------------------------//
// sort                                                                     //
// Returns a sorted index of input array of strings with 'count'            //
// elements. Does not sort the input array itself. (Comb sort)              //
//--------------------------------------------------------------------------//
void sort(char *item[], int sorted[], int count)
{
    int k,gap,iswitch,ii,jj;
    gap = count;
    for(k=0;k<count;k++) sorted[k] = k;
    do
    {  gap = max((int)(gap/1.3), 1);
       iswitch = 0;
       for(ii=0;ii<count-gap;ii++)
       {  jj = ii + gap;
          if(strcmp(item[sorted[ii]], item[sorted[jj]]) > 0)
          {  swap(sorted[ii],sorted[jj]);
             iswitch++;
          }   
       }
    } while((iswitch!=0)||(gap!=1));   
}      


//--------------------------------------------------------------------------//
// sort                                                                     //
// Returns a sorted index of input array of ints with 'count'               //
// elements. Does not sort the input array itself. (Comb sort)              //
//--------------------------------------------------------------------------//
void sort(int item[], int sorted[], int count)
{
    int k,gap,iswitch,ii,jj;
    gap = count;
    for(k=0;k<count;k++) sorted[k] = k;
    do
    {  gap = max((int)(gap/1.3), 1);
       iswitch = 0;
       for(ii=0;ii<count-gap;ii++)
       {  jj = ii + gap;
          if(item[sorted[ii]] > item[sorted[jj]])
          {  swap(sorted[ii],sorted[jj]);
             iswitch++;
          }   
       }
    } while((iswitch!=0)||(gap!=1));   
}      



//--------------------------------------------------------------------------//
// sort (for doubles) - should replace with template                        //
//--------------------------------------------------------------------------//
void sort(double item[], int sorted[], int count)
{
    int k,gap,iswitch,ii,jj;
    gap = count;
    for(k=0;k<count;k++) sorted[k] = k;
    do
    {  gap = max((int)(gap/1.3), 1);
       iswitch = 0;
       for(ii=0;ii<count-gap;ii++)
       {  jj = ii + gap;
          if(item[sorted[ii]] > item[sorted[jj]])
          {  swap(sorted[ii], sorted[jj]);
             iswitch++;
          }   
       }
    } while((iswitch!=0)||(gap!=1));   
}      


//--------------------------------------------------------------------------//
// help                                                                     //
// Reads 'tnimage.hlp' and provides context-sensitive help.                 //
// Puts text in a list box.                                                 //
//--------------------------------------------------------------------------//
void help(int topic)
{
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  static char **help_info;                                    
  static listinfo *help_list;
  static char *help_title;
  help_title = new char[100];
  help_info = new char*[4096];     // Maximum of 4096 lines of help
  help_list = new listinfo;
  int tno;
  int intopic=0;
  FILE *fp;
  char tempstring[FILENAMELENGTH];

  //// Variables for list box
  int listcount;
  int firstitem=0;
  int helptopic=34;

  if(topic==0) topic=1;
  if(topic<=-1)
  {   if(g.imageatcursor>0) { show_notes(ci); return; }
      else topic = 1;
  }
  sprintf(tempstring, "%s/%s",g.helpdir,"tnimage.hlp");
  if ((fp=fopen(tempstring,"r")) == NULL)
  {   sprintf(tempstring, "%s/%s",g.homedir,"tnimage.hlp");
      if ((fp=fopen(tempstring,"r")) == NULL)
      {   if((fp=fopen("tnimage.hlp","r")) == NULL)
          {   sprintf(tempstring, "Can't find help file\n(Should be: %s/tnimage.hlp)",g.helpdir);
              message (tempstring,3);
              return;
          }
      }
  }
  tno=0;
  intopic=0;
  while(!feof(fp))             // Count no. of lines in topic
  {  fgets(tempstring,85,fp); 
     if(isdigit(tempstring[0])) tno = atoi(tempstring);
     if(tno==topic) intopic=1; 
     if((intopic) && (tno) && (tno!=topic)) break;
  }
  intopic=0;
  tno=0;

  fseek(fp,0,SEEK_SET);
  listcount = 0;
  while(!feof(fp))
  {  fgets(tempstring,85,fp); 
     if(isdigit(tempstring[0])) tno = atoi(tempstring);
     if(tno==topic) intopic=1;
     if((intopic) && (tno) && (tno!=topic)) break;
     if(intopic)
     {   tempstring[80] =0;
         remove_terminal_cr(tempstring);
         help_info[listcount] = new char[100];
         help_info[listcount][0]=0;
         if(strlen(tempstring)) strcpy(help_info[listcount], tempstring);
         else strcpy(help_info[listcount]," ");
         listcount++;
     }
     if(listcount>1023) break;
  }
  fclose(fp);
  
  if(listcount)
  {  strcpy(help_title, help_info[0]);
     help_list->title = help_title;
     help_list->info  = help_info;
     help_list->count = listcount;
     help_list->itemstoshow = min(10, listcount);
     help_list->firstitem   = firstitem;
     help_list->wantsort    = 0;
     help_list->wantsave    = 0;
     help_list->helptopic   = helptopic;
     help_list->allowedit   = 0;
     help_list->selection   = &g.crud;
     help_list->width       = 0;
     help_list->transient   = 1;
     help_list->wantfunction = 0;
     help_list->autoupdate   = 0;
     help_list->clearbutton  = 0;
     help_list->highest_erased = 0;
     help_list->f1 = null; // use f1 to perform an action when item is selected
     help_list->f2 = null; // use f2 to update list 
     help_list->f3 = null;
     help_list->f4 = delete_list;
     help_list->f5 = null;
     help_list->f6 = null;
     help_list->listnumber = 0;
     list(help_list);
  }
}


//--------------------------------------------------------------------------//
// show_notes                                                               //
//--------------------------------------------------------------------------//
void show_notes(int ino)
{
  FILE *fp;
  static char filename[FILENAMELENGTH];
  sprintf(filename, "%s/notes/%s", g.homedir, basefilename(z[ino].name));
  int fsize = filesize(filename);
  if(z[ino].notes == NULL)
  {   
      z[ino].notes = new char[65536];
      if(z[ino].notes == NULL)
      {   message(g.nomemory); 
          return; 
      }
      z[ino].notes[0] = 0;
      if(fsize==0 || access(filename, F_OK)) // access = 0 if file exists
      {
          message("No image notes found"); 
      }else
      {   if((fp=fopen(filename,"r")) == NULL)
          {   error_message(filename, errno);
              return;
          }
          if(fread(z[ino].notes, fsize, 1, fp) == 0)
          {   message("Notes string had zero length"); 
              delete z[ino].notes; 
              z[ino].notes = NULL; 
              fclose(fp);
              return; 
          }
          fclose(fp);
      }
  }
  edit("Image notes", "Image notes", z[ino].notes, 20, 80, 0,0, 65536, 
      70, 0, filename, null, NULL);
}




