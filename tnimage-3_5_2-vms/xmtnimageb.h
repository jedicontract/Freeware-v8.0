//-----------------------------------------------------------------------//
//  xmtnimageb.h                                                         //
//  Latest revision: 08-13-1997                                          //
//  Copyright (C) 1997 by Thomas J. Nelson                               //
// See xmtnimage.h for Copyright Notice                                  //
//  1d, 2d, 3d, and 4d array classes                                     //
//  The array can be accessed using [][] array notation (without multi-  //
//  plications) or using [] notation for compatibility with Xlib and     //
//  other C libraries.  It can also be moved by memmove() and memcpy().  //
//  Note: These classes are largely untested, use at your own risk.      //
//-----------------------------------------------------------------------//

template<class type> class array
{
    private:
    public:
       int xsize;                     // No.of columns in array
       int ysize;                     // No.of rows
       int zsize;                     // In case of 3d array
       int tsize;                     // In case of 4d array
       int elements;                  // Total no. of elements
       int allocated;                 // 1=data were allocated
      
       type *data;                    // The 2d data, as a 1d array.
       type **p;                      // The 2d data which can be referred
                                      // to using [][].
       type ***q;                     // The 3d data which can be referred
                                      // to using [][][] if zsize is specified.
       type ****r;                    // The 4d data which can be referred
                                      // to using [][][][] if tsize is specified.
       array(int x);         
       array(int x, int y); 
       array(int x, int y, int z);   
       array(int x, int y, int z, int t);   
          
       array(const array<type> &other); // Copy constructor                                     

      ~array(void);                   // Destructor (defined below)

       void freearray();              // Free array space

       void print(void);  
                                      // Overload the = operator so you can
                                      // say a=b, where a and b are arrays.                               
       const array<type> &operator=(const array<type> &s); 

                                      // Overloaded = operator for initializ-
                                      // ing an array to a constant value.
       const array<type> &operator= (const int &a);

                                      // Overload the + operator so you can
                                      // add two arrays together with c=a+b.
                                      // You also may need to add an integer.
                                      // Must have - * and / for completeness.
       array operator+(const array &s);
       array operator*(const array &s);
       array operator-(const array &s);
       array operator/(const array &s);
       array operator+ (const int &a);
       array operator* (const int &a);
       array operator- (const int &a);
       array operator/ (const int &a);

                                      // Ensure commutativity by overloading
                                      // the operators for the reverse direc-
                                      // tion. If you use 'friend' it puts the
                                      // 2 operands in parentheses and returns
                                      // the type specified (array) so the
                                      // 1st operand can be something other
                                      // than an 'array' type. Making it
                                      // 'inline' makes it faster.
                                      
       inline friend array operator+ (int &a , array &s) {  return s + a;  }
       inline friend array operator- (int &a , array &s) {  return s - a;  }
       inline friend array operator* (int &a , array &s) {  return s * a;  }
       inline friend array operator/ (int &a , array &s) {  return s / a;  }
      
};


//---------------Member functions for array class----------------------//           


template<class type> 
array<type>::array(int x) : xsize(0), ysize(0), zsize(0), tsize(0), 
    elements(0), allocated(0), data(0), p(0), q(0), r(0)
{
    xsize = x;
    ysize = 0;
    zsize = 0;
    tsize = 0;
    elements = x;
    data = new type[elements];     
    if(data!=0) allocated = 1;
    r = NULL;
    q = NULL;
    p = NULL;   
} 


// Constructor taking 2 arguments
template<class type> 
array<type>::array(int x, int y) : xsize(0), ysize(0), zsize(0), tsize(0), 
    elements(0), allocated(0), data(0), p(0), q(0), r(0)
{
    xsize = x;
    ysize = y;
    zsize = 0;
    tsize = 0;
    elements = x*y;
    data = new type[elements];   
    if(data!=0) allocated = 1;
    p = new type *[ysize];  
    q = NULL;
    r = NULL;
    for(int k=0;k<ysize;k++)  p[k] = data + k*xsize;
}


/*
   Example:  array<double> buf(x,y,z)  
             buf[z][y][x] = 4;       
*/
// Constructor taking 3 arguments
template<class type> 
array<type>::array(int x, int y, int z) : xsize(0), ysize(0), zsize(0), tsize(0), 
    elements(0), allocated(0), data(0), p(0), q(0), r(0)
{
    tsize = 0;
    xsize = x;
    ysize = y;
    zsize = z;
    elements = x*y*z;
    data = new type[elements];  
    if(data!=0) allocated = 1;
    p = NULL;
    q = new type **[zsize];    
    r = NULL;
    for(int j=0;j<zsize;j++)  
    {     q[j] = new type*[ysize];
          for(int k=0;k<ysize;k++)
               q[j][k] = data + j*xsize*ysize + k*xsize;
    }
}


// Constructor taking 4 arguments
template<class type> 
array<type>::array(int x, int y, int z, int t) : xsize(0), ysize(0), zsize(0), tsize(0), 
    elements(0), allocated(0), data(0), p(0), q(0), r(0)
{
    tsize = t;
    xsize = x;
    ysize = y;
    zsize = z;
    elements = t*x*y*z;
    data = new type[elements];  
    if(data!=0) allocated = 1;
    p = NULL;
    q = NULL;
    r = new type ***[tsize];
    for(int i=0;i<tsize;i++)
    {     r[i] = new type **[zsize];
          for(int j=0;j<zsize;j++)  
          {     r[i][j] = new type*[ysize];
                for(int k=0;k<ysize;k++)
                    r[i][j][k] = data + i*xsize*ysize*zsize + j*xsize*ysize + k*xsize;
          }
    }
}


template<class type> 
void array<type>::freearray()                    // Free array space
{
    int i,j;
    if(!allocated) return;
    if(tsize) 
    {     for(i=0;i<zsize;i++)
          {     for(j=0;j<zsize;j++) delete[] r[i][j];
                delete[] r[i];
          }
          delete[] r;
    }else if(zsize) 
    {     for(j=0;j<zsize;j++) delete[] q[j];
          delete[] q;  
    }else if(xsize) 
          delete[] p;
    delete[] data;
    allocated = 0;
}


template<class type> 
array<type>::~array(void)
{   
    freearray();
}    


template<class type> 
void array<type>::print(void)
{
    int h,i,j,k;
    if(tsize)  
       for(h=0;h<tsize;h++)
       for(i=0;i<zsize;i++)
       for(j=0;j<ysize;j++)
       for(k=0;k<xsize;k++)
       printf("%d ",r[h][i][j][k]); 
    else if(zsize) 
       for(i=0;i<zsize;i++)
       for(j=0;j<ysize;j++)
       for(k=0;k<xsize;k++)
       printf("%d ",q[i][j][k]); 
    else  
       for(j=0;j<ysize;j++)
       for(k=0;k<xsize;k++)
       printf("%d ",p[j][k]); 
    printf("\n");
}


template<class type> 
array<type>::array(const array &other)     // Copy constructor not finished dont use
{                                     
    int i,j;  
    if(other.tsize || other.zsize) 
         printf("Error in file %s line %d\n",__FILE__,__LINE__);
    xsize=other.xsize;
    ysize=other.ysize;
    array buf(xsize,ysize);
    for(i=0;i<xsize;i++)
    for(j=0;j<ysize;j++) buf.p[i][j]=other.p[i][j];
}


template<class type> 
const array<type> &array<type>::operator= (const array &s)  // Not finished, dont use
{     
    if(s.tsize || s.zsize) printf("Error in file %s line %d\n",__FILE__,__LINE__);
    if(elements!=s.elements)           // If different size, replace the buffer
    {                                  // with one that's the same size as the
        delete data;                   // source.
        elements=s.elements;
        data = new type[elements];
    }                                 // If you didn't overload '=', you can
                                      // still say 'a=b' but if they are 
                                      // different sizes it overwrites memory.
           
    for(int k=0;k<elements;k++) data[k]=s.data[k];
    return *this;                     // Use 'return *this' so it can also say
                                      // 'a=b=c'. 
}


template<class type> 
const array<type> &array<type>::operator= (const int &a)
{                                     // Overload the '=' operator so you can
                                      // say 'a=k' for 1 array + a constant.
                                      // Return a reference so it doesn't have
                                      // to make a copy of it.
    for(int k=0;k<elements;k++) data[k]=a;
    return *this;    
}

template<class type> 
inline array<type> array<type>::operator+ (const array &s)
{
    array temp(elements);
    for(int k=0;k<elements;k++) temp.data[k] = s.data[k] + data[k];
    return temp;   
}


template<class type> 
array<type> array<type>::operator+ (const int &a)
{
    array temp(elements);
    for(int k=0;k<elements;k++) temp.data[k] = data[k] + a;
    return temp;   
}

template<class type> 
array<type> array<type>::operator* (const int &a)
{
    array temp(elements);
    for(int k=0;k<elements;k++) temp.data[k] = data[k] * a;
    return temp;   
}

template<class type> 
array<type> array<type>::operator- (const int &a)
{
    array temp(elements);
    for(int k=0;k<elements;k++) temp.data[k] = data[k] - a;
    return temp;   
}

template<class type> 
array<type> array<type>::operator/ (const int &a)
{
    if(a)
    {    array temp(elements);
         for(int k=0;k<elements;k++) temp.data[k] = data[k] / a;
         return temp;   
    }
    else
    {    printf("Division by zero!\n");
         return *this;
    }
}

template<class type> 
array<type> array<type>::operator* (const array &s)
{
    array temp(elements);
    for(int k=0;k<elements;k++) temp.data[k] = s.data[k] * data[k];
    return temp;   
}

template<class type> 
array<type> array<type>::operator- (const array &s)
{
    array temp(elements);
    for(int k=0;k<elements;k++) temp.data[k] = s.data[k] - data[k];
    return temp;   
}

template<class type> 
array<type> array<type>::operator/ (const array &s)
{
    array temp(elements);
    for(int k=0;k<elements;k++)
    {  if(data[k]) temp.data[k] = s.data[k] / data[k];
       else { printf("Division by 0\n"); temp.data[k]=0; }
    }   
    return temp;   
}


