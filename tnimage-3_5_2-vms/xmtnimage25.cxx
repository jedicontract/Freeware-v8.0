//--------------------------------------------------------------------------//
// xmtnimage25.cc                                                           //
// IPC                                                                      //
// Latest revision: 06-24-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// Includes fix for creating fft in a plugin, from Dietmar Kunz             //
// <dkunz@server2.fo.FH-Koeln.DE>.                                          //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

int s_pipe(int fd[2]);
int pluginmode=1;

//--------------------------------------------------------------------------//
//  select_plugin                                                           //
//  mode = 0 (test plugin) or 1 (execute plugin). In testing mode, the      //
//  plugin writes to stdout to allow user to debug it.                      //
//  In a stream pipe, each channel is bidirectional. The child reads and    //
//  writes to fd[1] and the parent reads and writes to fd[0].               //
//--------------------------------------------------------------------------//
int select_plugin(int noofargs, char **arg)
{
  static char *pluginlist_title;
  static char **pluginlist_info;                                    
  static listinfo *pluginlist;
  static int pluginlist_selection;

  pluginlist = new listinfo;
  pluginlist_title = new char[100];
  strcpy(pluginlist_title, "Select plugin");
  pluginlist_info = new char*[1024];            // Maximum of 1024 plugins
  pluginmode=1;
  FILE *fp;
  int k, count=0;
  if(ci<0) { message("Please select an image",ERROR); return(NOIMAGES); }
  char tempstring[FILENAMELENGTH];
  char pluginfile[FILENAMELENGTH];
  if(noofargs==0)
  {   sprintf(pluginfile,"%s/%s", g.helpdir, "plugins");
      if((fp=fopen(pluginfile,"r")) == NULL)
      {   sprintf(pluginfile,"%s/%s",g.homedir,"plugins");
          if((fp=fopen(pluginfile,"r")) == NULL)
          {   error_message(pluginfile, errno);
              sprintf(tempstring, "Can't find list of plugins\nshould be %s",pluginfile);
              message(tempstring,ERROR);
              return ERROR;
          }
      }
      while(!feof(fp))             // Count no. of plugins
      {    fgets(tempstring,FILENAMELENGTH-1,fp); 
           if(strlen(tempstring)==0) break;
           if(tempstring[0]==10) break;
           tempstring[100]=0;
           pluginlist_info[count] = new char[100]; 
           for(k=0;k<(int)strlen(tempstring);k++) 
               if(tempstring[k]==10) tempstring[k]=0;
           strcpy(pluginlist_info[count], tempstring);
           if(feof(fp))break;
           count++;
      }
      fclose(fp);
 
      sprintf(pluginfile,"%s%s",g.homedir,"/plugins");
      if((fp=fopen(pluginfile,"r")) != NULL)
      {    while(!feof(fp))             // Count no. of plugins
           {    fgets(tempstring,FILENAMELENGTH-1,fp); 
                if(strlen(tempstring)==0) break;
                if(tempstring[0]==10) break;
                tempstring[100]=0;
                pluginlist_info[count] = new char[100]; 
                for(k=0;k<(int)strlen(tempstring);k++) 
                    if(tempstring[k]==10) tempstring[k]=0;
                strcpy(pluginlist_info[count],tempstring);
                if(feof(fp))break;
                count++;
           }
           fclose(fp);
      }
      pluginlist->title = pluginlist_title;
      pluginlist->info  = pluginlist_info;
      pluginlist->count = count;
      pluginlist->itemstoshow = 8;
      pluginlist->firstitem   = 0;
      pluginlist->wantsort    = 0;
      pluginlist->wantsave    = 0;
      pluginlist->helptopic   = 0;
      pluginlist->selection   = &pluginlist_selection;
      pluginlist->allowedit   = 1;
      pluginlist->edittitle   = new char[100];
      strcpy(pluginlist->edittitle, "Enter arguments for plugin");
      pluginlist->editrow    = 0;
      pluginlist->editcol    = 0;
      pluginlist->maxstringsize = 99;
      pluginlist->width       = 0;
      pluginlist->transient   = 1;    
      pluginlist->wantfunction = 1;
      pluginlist->autoupdate   = 0;
      pluginlist->clearbutton  = 0;
      pluginlist->highest_erased = 0;
      pluginlist->f1 = plugincheck;
      pluginlist->f2 = null;
      pluginlist->f3 = null;
      pluginlist->f4 = delete_list;
      pluginlist->f5 = null;
      pluginlist->f6 = null;
      pluginlist->listnumber = 0;
      list(pluginlist);
  }else
  {  
      if(noofargs>=1) execute_plugin(arg[1], &arg[1], max(0,noofargs-1), pluginmode);
  }
  return OK;
}

    
//--------------------------------------------------------------------------//
//  plugincheck                                                             //
//--------------------------------------------------------------------------//
void plugincheck(listinfo *l)
{
  const int CMDLEN=128;
  int k, noofargs;
  int selection = *l->selection - l->firstitem;
  char **arg;                // up to 20 arguments
  char command[FILENAMELENGTH];
  arg = new char*[20];
  for(k=0; k<20; k++) arg[k] = new char[CMDLEN];

  strcpy(command, l->info[selection]);
  if(g.getout){ g.getout=0; return; }
  parsecommand(command, FORMULA, arg, noofargs, CMDLEN, 20);
  execute_plugin(arg[0], arg, noofargs, pluginmode);

  for(k=0; k<20; k++) delete[] arg[k];
  delete arg;  
}


//--------------------------------------------------------------------------//
//  execute_plugin                                                          //
//--------------------------------------------------------------------------//
int execute_plugin(char *filename, char **arg, int noofargs, int mode)
{
   enum{ TEST, EXECUTE };
   int count, k, n=0, fd[2], have_message=0, size,x1,y1,x2,y2, oldfft=0, newfft=0;
   int oldxsize, oldysize, erase, newino=-1, oldwave, newwave, oldtype, oldlevels;
   pid_t pid;
   char tempstring[1024];
   char zero[10] = {0,0,0,0,0,0,0,0,0,0};
   int changed[MAXIMAGES];
   int oimagecount = g.image_count;
   if(ci>=0) {  x1=0;     y1=0;     x2=z[ci].xsize; y2=z[ci].ysize;  }
   else      {  x1=g.ulx; y1=g.uly; x2=g.lrx;       y2=g.lry;  }

   if(g.busy) return BUSY; // Already running a plugin
   g.busy = 1;             // No deleting images past here
   if(signal(SIGPIPE, sig_pipe) == SIG_ERR) fprintf(stderr, "signal error\n");
   if(signal(SIGCHLD, sig_child) == SIG_ERR) fprintf(stderr, "sigchld\n");
   if(s_pipe(fd)<0) fprintf(stderr, "pipe error\n");
   if((pid=fork())<0) fprintf(stderr, "fork error\n");
   else if(pid>0)    // Parent
   {   
        close(fd[1]);

        ////  Up to 20 command line arguments length up to 128
        ////  separated by spaces
        for(k=0;k<=noofargs;k++)
        {  n+=write(fd[0], arg[k], strlen(arg[k]));
           n+=write(fd[0], " ", 1);
        }

        ////  Send currently-selected area as command line arguments
        sprintf(tempstring,"%d ",x1);        
        n+=write(fd[0], tempstring, strlen(tempstring));
        sprintf(tempstring,"%d ",y1); 
        n+=write(fd[0], tempstring, strlen(tempstring));
        sprintf(tempstring,"%d ",x2); 
        n+=write(fd[0], tempstring, strlen(tempstring));
        sprintf(tempstring,"%d ",y2); 
        n+=write(fd[0], tempstring, strlen(tempstring));

        ////  Mark the end of command-line arguments with a null
        n+=write(fd[0], zero, 1);

        ////  Current image no.
        write(fd[0], &ci, sizeof(int));
        
        ////  'mode' can be TEST or EXECUTE
        write(fd[0], &mode, sizeof(mode));

        ////  No. of images present
        write(fd[0], &g.image_count,  sizeof(g.image_count));

        ////  Current screen depth
        write(fd[0], &g.bitsperpixel, sizeof(g.bitsperpixel));

        ////  Send image params.
        ////  Send all the images so plugin can do multi-image calculations.
        for(k=1;k<g.image_count;k++)
        {
            changed[k]=0;
            write(fd[0], &z[k], sizeof(z[k]));
            size = z[k].frames * z[k].xsize * z[k].ysize * g.off[z[k].bpp];
            write_fd(z[k].image_1d, fd[0], size);
            if(z[k].floatexists)
            {     size = z[k].fftxsize * z[k].fftysize * sizeof(complex);
                  write_fd((uchar *)z[k].fft_1d, fd[0], size);
            }
            if(z[k].waveletexists)
            {     size = z[k].wavexsize * z[k].waveysize * sizeof(double);
                  write_fd((uchar *)z[k].wavelet_1d, fd[0], size);
            }
            write(fd[0], z[k].palette, 768);
            write(fd[0], z[k].opalette, 768);
            write(fd[0], z[k].spalette, 768);
            write(fd[0], z[k].name, FILENAMELENGTH);
            write(fd[0], z[k].cal_title[0], FILENAMELENGTH);
            write(fd[0], z[k].cal_title[1], FILENAMELENGTH);
            //// no cal_title[2] for compatibility
            write(fd[0], z[k].gamma, 256*sizeof(short int));
        }

        ///////////////////////////////////////////////////////////////////
        ////  Finished writing data - block here until input comes in  ////
        ///////////////////////////////////////////////////////////////////

        ////  Read back and display error message from plugin

        n=read(fd[0], &have_message, sizeof(int));
        n=read(fd[0], tempstring, 1024);

        ////  These lines should match `write_data()' in plugin.
        n=read(fd[0], &ci, sizeof(int));
        n=read(fd[0], &g.ulx, sizeof(int));
        n=read(fd[0], &g.uly, sizeof(int));
        n=read(fd[0], &g.lrx, sizeof(int));
        n=read(fd[0], &g.lry, sizeof(int));
        n=read(fd[0], &g.image_count, sizeof(int));
        count = g.image_count;

        ////  Can't read z[k] because it contains pointers.
        for(k=1; k<count; k++)
        {  
             n=read(fd[0], &changed[k], sizeof(int));
             n=read(fd[0], &z[k].xpos, sizeof(z[k].xpos));
             n=read(fd[0], &z[k].ypos, sizeof(z[k].ypos));
             n=read(fd[0], &z[k].xsize, sizeof(z[k].xsize));
             n=read(fd[0], &z[k].ysize, sizeof(z[k].ysize));
             n=read(fd[0], &z[k].bpp, sizeof(z[k].bpp));
             n=read(fd[0], &z[k].frames, sizeof(z[k].frames));
             n=read(fd[0], &z[k].colortype, sizeof(z[k].colortype));

             ////  If they created a new image
             newino = -1;
             if(k>=oimagecount)
             {   ////  This changes g.image_count and ci
                 g.image_count--;
                 newimage(basefilename(filename),0,0,z[k].xsize,z[k].ysize,
                       z[k].bpp,z[k].colortype,
                       z[k].frames,g.want_shell,1,PERM,1,g.window_border,0);
                 newino = ci;
             }
             size = z[k].frames * z[k].xsize * z[k].ysize * g.off[z[k].bpp];
             n=read_fd(z[k].image_1d, fd[0], size);

             ////  If they created or erased an fft, or if they changed the size
             ////  (not a good idea), erase old one

             oldfft = z[k].floatexists;
             n=read(fd[0], &z[k].floatexists, sizeof( int ));             
             newfft = z[k].floatexists;
             oldxsize = z[k].fftxsize;
             oldysize = z[k].fftysize;

             //// Make conditional for back compatibility
             if(newfft)
             {
                   n=read(fd[0], &z[k].fftxsize, sizeof(int));
                   n=read(fd[0], &z[k].fftysize, sizeof(int));
             }

             //// If they changed the size or erased the buffer, 
             //// erase old buffer first

             erase = 0;
             if( oldfft &&
                ( !newfft || z[k].fftxsize != oldxsize || z[k].fftysize != oldysize ) )
                erase = 1;
             if(erase) erase_fft(k);   // This sets floatexists to 0
             z[k].floatexists = newfft;
             
             if(newfft)
             {
                   size = z[k].fftxsize * z[k].fftysize * sizeof(complex);
                   if( erase || !oldfft )
                   {
                        z[k].fft_1d = (complex*) malloc(z[k].fftysize*z[k].fftxsize*sizeof(complex));
                        if(z[k].fft_1d==NULL) message("Error allocating fft matrix",ERROR);
                        z[k].fft = make_2d_alias(z[k].fft_1d,z[k].fftxsize,z[k].fftysize);
                        if(z[k].fft==NULL)
                        {    message(g.nomemory,ERROR);
                             close(fd[0]);
                             g.busy = 0;
                             return(NOMEM);
                        }
                   }
                   read_fd((uchar *)z[k].fft_1d, fd[0], size);
             }

             ////  The same story for the wavelet buffer
             ////

             oldwave = z[k].waveletexists;
             n=read(fd[0], &z[k].waveletexists, sizeof( int ));
             newwave = z[k].waveletexists;
             oldxsize = z[k].wavexsize;
             oldysize = z[k].waveysize;
             oldtype = z[k].wavelettype;
             oldlevels = z[k].nlevels;

             if(newwave)
             {
                   n=read(fd[0], &z[k].wavexsize, sizeof(int));
                   n=read(fd[0], &z[k].waveysize, sizeof(int));
                   n=read(fd[0], &z[k].origxsize, sizeof(int));
                   n=read(fd[0], &z[k].origysize, sizeof(int));
                   n=read(fd[0], &z[k].wavelettype, sizeof(int));
                   n=read(fd[0], &z[k].nlevels, sizeof(int));
             }

             //// If they changed the size or erased the buffer,
             //// erase old buffer first

             erase = 0;
             if( oldwave &&
                ( !newwave || z[k].wavexsize != oldxsize || z[k].waveysize != oldysize
                || z[k].wavelettype != oldtype || z[k].nlevels != oldlevels ) )
                erase = 1;
             if(erase) erase_wavelet(k);   // This sets waveletexists to 0
             z[k].waveletexists = newwave;

             if(newwave)
             {
                   size = z[k].wavexsize * z[k].waveysize * sizeof(double);
                   if( erase || !oldwave )
                   {
                        allocate_image_wavelet
                           (k, z[k].wavexsize, z[k].waveysize, z[k].nlevels, z[k].wavelettype);
                   }
                   read_fd((uchar *)z[k].wavelet_1d, fd[0], size);
             }
        
             n=read(fd[0], &z[k].backedup, sizeof(int));
             n=read(fd[0], &z[k].fftdisplay, sizeof(int));
             n=read(fd[0], &z[k].fmin, sizeof(double));
             n=read(fd[0], &z[k].fmax, sizeof(double));
             n=read(fd[0], &z[k].wmin, sizeof(double));
             n=read(fd[0], &z[k].wmax, sizeof(double));
             n=read(fd[0], z[k].gray, 4*sizeof(int));
             n=read(fd[0], &z[k].fftstate, sizeof(int));
             n=read(fd[0], z[k].palette, 768);

             n=read(fd[0], z[k].opalette, 768);
             n=read(fd[0], z[k].spalette, 768);
             n=read(fd[0], z[k].name,  FILENAMELENGTH);
             n=read(fd[0], z[k].cal_title[0], FILENAMELENGTH);
             n=read(fd[0], z[k].cal_title[1], FILENAMELENGTH);
             //// no cal_title[2] for compatibility
             n=read(fd[0], z[k].gamma, 256*sizeof(short int));
             if(newino > -1) switchto(newino);
        }
        sleep(2);   
        wait(0);    // Wait for child process to terminate
        close(fd[0]);
        if(n==0) message("Child closed pipe",ERROR);
        if(ferror(stdin)) message("fgets error on stdin",ERROR);

   }else            // Child
   { 
        close(fd[0]);
        if(fd[1] != STDIN_FILENO)
        {    if(dup2(fd[1], STDIN_FILENO) != STDIN_FILENO)
                 fprintf(stderr, "dup2 error to stdin\n");
        }
        if(fd[1] != STDOUT_FILENO && mode==EXECUTE)
        {    if(dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
             fprintf(stderr, "dup2 error to stdout\n");
        }
        if(execl(filename, filename, NULL) <0) fprintf(stderr, "execl error\n");
        close(fd[1]);
        _exit(0);     // Child must exit
   }

   g.busy = 0;
   if(have_message) message(tempstring);
   drawselectbox(OFF);
   for(k=1; k<g.image_count; k++) 
   {  if(changed[k])
      {   switchto(k);
          repair(k);
      }
   }
   return OK;
}    

        
//--------------------------------------------------------------------------//
//  sig_pipe                                                                //
//--------------------------------------------------------------------------//
void sig_pipe(int signo)
{
   fprintf(stderr, "Caught SIGPIPE (%d), crashing\n",signo);
   exit(1);
}


//--------------------------------------------------------------------------//
//  sig_child                                                               //
//--------------------------------------------------------------------------//
void sig_child(int signo)
{
   signo=signo;
   char tempstring[1024];
   int statloc;
   wait(&statloc);
   sprintf(tempstring, "Process returned %d\n", statloc);
   if(WIFEXITED(statloc)) strcat(tempstring,"(Successful)");
   else if(WIFSIGNALED(statloc)) strcat(tempstring,"(Abnormal termination)"); 
   else if(WIFSTOPPED(statloc)) strcat(tempstring,"(Stopped)");  
   else if(statloc) strcat(tempstring,"(Unsuccessful)");  
   message(tempstring);
}


//--------------------------------------------------------------------------//
//  s_pipe - open bidirectional BSD style stream pipe                       //
//--------------------------------------------------------------------------//
int s_pipe(int fd[2])
{
    return(socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
}
 


//--------------------------------------------------------------------------//
//  write_fd - write to a file descriptor. Can only send 62000 bytes at a   //
//  time so break it up into <=1k chunks.  Returns bytes written.           //
//--------------------------------------------------------------------------//
int write_fd(uchar *source, int fd, int size)
{
   int n;
   int total=0;        // bytes sent
   int rem = size;     // bytes remaining to send
   while(total<size)   
   {     n = write(fd, source + total, min(rem,1024)); 
         if(n<=0){ message("Error writing to plugin"); break; }
         total += n;
         rem -= n;
   }
   return total;
}


//--------------------------------------------------------------------------//
//  read_fd - read from a file descriptor. Can only read 62000 bytes at a   //
//  time so break it up into <=1k chunks.  Returns bytes read.              //
//--------------------------------------------------------------------------//
int read_fd(uchar *dest, int fd, int size)
{
   int n;
   int total=0;        // bytes read
   int rem = size;     // bytes remaining to read
   while(total<size)   
   {    n = read(fd, dest + total, min(rem,1024));
        if(n<=0){ message("Error reading from plugin"); break; }
        total += n;
        rem -= n;
   }
   return total;
}

