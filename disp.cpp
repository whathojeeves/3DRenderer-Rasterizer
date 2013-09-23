/*   CS580 HW   */
#include    "stdafx.h"  
#include	"Gz.h"
#include	"disp.h"
#include	<fstream>

using namespace std;

int clampPixel(GzIntensity *pix)
{
	if(*pix < 0)
		*pix=0;
	else if(*pix >4095)
		*pix=4095;

	return GZ_SUCCESS;
}

int GzNewFrameBuffer(char** framebuffer, int width, int height)
{
/* create a framebuffer:
 -- allocate memory for framebuffer : (sizeof)GzPixel x width x height
 -- pass back pointer 
 -- NOTE: this function is optional and not part of the API, but you may want to use it within the display function.
*/
	int sizeAlloc = 0;
	sizeAlloc = 3*width*height;
	*framebuffer = (char*)malloc(sizeAlloc);
	if(*framebuffer)
	{
		return GZ_SUCCESS;
	}
	return GZ_FAILURE;
}

int GzNewDisplay(GzDisplay	**display, int xRes, int yRes)
{
/* create a display:
  -- allocate memory for indicated resolution
  -- pass back pointer to GzDisplay object in display
*/
	int sizeAlloc = 0;
	sizeAlloc = sizeof(GzDisplay);
	*display = (GzDisplay*)malloc(sizeAlloc);
	if(*display)
	{
		(*display)->xres = xRes;
		(*display)->yres = yRes;
		(*display)->fbuf = (GzPixel*)malloc(sizeof(GzPixel)*xRes*yRes);
		return GZ_SUCCESS;
	}
	return GZ_FAILURE;
}


int GzFreeDisplay(GzDisplay	*display)
{
/* clean up, free memory */
	free(display);
	return GZ_SUCCESS;
}


int GzGetDisplayParams(GzDisplay *display, int *xRes, int *yRes)
{
/* pass back values for a display */
	*xRes = display->xres;
	*yRes = display->yres;
	return GZ_SUCCESS;
}


int GzInitDisplay(GzDisplay	*display)
{
/* set everything to some default values - start a new frame */
	memset(display->fbuf,0,sizeof(display->fbuf));
	int pos=0;

	for(int j=0;j<display->yres;j++)
	{
		for(int i=0;i<display->xres;i++)
		{
			pos = i+(j*(display->xres));
			display->fbuf[pos].z = INT_MAX;
		}
	}
	return GZ_SUCCESS;
}


int GzPutDisplay(GzDisplay *display, int i, int j, GzIntensity r, GzIntensity g, GzIntensity b, GzIntensity a, GzDepth z)
{
/* write pixel values into the display */
	int xres = 0,yres = 0;
	if(GzGetDisplayParams(display,&xres,&yres) == GZ_FAILURE)
	{
		return GZ_FAILURE;
	}
	if((i<0 || j<0) || (i>(xres-1) || j>(yres-1)) || (i+(j*(xres)) > xres*yres))
	{
		return GZ_FAILURE;
	}
	clampPixel(&r);
	clampPixel(&g);
	clampPixel(&b);
	clampPixel(&a);

	int pos = i+(j*(xres));
	GzPixel* pixelBuf = display->fbuf;
	pixelBuf[pos].alpha = a;
	pixelBuf[pos].blue = b;
	pixelBuf[pos].green = g;
	pixelBuf[pos].red = r;
	pixelBuf[pos].z = z;

	return GZ_SUCCESS;
}


int GzGetDisplay(GzDisplay *display, int i, int j, GzIntensity *r, GzIntensity *g, GzIntensity *b, GzIntensity *a, GzDepth *z)
{
	/* pass back pixel value in the display */
	int xres = 0,yres = 0;
	if(GzGetDisplayParams(display,&xres,&yres) != GZ_SUCCESS)
	{
		return GZ_FAILURE;
	}
	if(i+(j*(xres)) > xres*yres)
	{
		return GZ_FAILURE;
	}

	int pos = i+(j*(xres));
	GzPixel* pixelBuf = display->fbuf;

	*a = pixelBuf[pos].alpha;
	*b = pixelBuf[pos].blue;
	*g = pixelBuf[pos].green;
	*r = pixelBuf[pos].red;
	*z = pixelBuf[pos].z;
	
	return GZ_SUCCESS;
}


int GzFlushDisplay2File(FILE* outfile, GzDisplay *display)
{

	/* write pixels to ppm file -- "P6 %d %d 255\r" */
	int xres = 0,yres = 0;
	int i=0,j=0,ctr=0;
	unsigned char r,g,b;
	GzIntensity rz,gz,bz,az;
	GzDepth zz;
	unsigned char *temp_colour_buf;
	if(GzGetDisplayParams(display,&xres,&yres) != GZ_SUCCESS)
	{
		return GZ_FAILURE;
	}

	temp_colour_buf = (unsigned char*)malloc((3*xres*yres));
	fprintf(outfile,"P6 %d %d 255\n",xres,yres);
	for(j=0;j<yres;j++)
	{
		for(i=0;i<xres;i++)
		{
			if(GzGetDisplay(display,i,j,&rz,&gz,&bz,&az,&zz) != GZ_SUCCESS)
			{
				return GZ_FAILURE;
			}

			r = (unsigned char)((unsigned short)rz >> 4);
		
			g = (unsigned char)((unsigned short)gz >> 4);
			
			b = (unsigned char)((unsigned short)bz >> 4);
			
			temp_colour_buf[ctr++] = r;
			temp_colour_buf[ctr++] = g;
			temp_colour_buf[ctr++] = b;
		}
	}
	fwrite((void*)temp_colour_buf,sizeof(unsigned char),3*xres*yres,outfile);
	//fprintf(outfile,"%s",temp_colour_buf);
	
	return GZ_SUCCESS;
}

int GzFlushDisplay2FrameBuffer(char* framebuffer, GzDisplay *display)
{

	/* write pixels to framebuffer: 
		- Put the pixels into the frame buffer
		- Caution: store the pixel to the frame buffer as the order of blue, green, and red 
		- Not red, green, and blue !!!
	*/

	int xres = 0,yres = 0;
	int i=0,j=0,ctr=0;
	unsigned char r,g,b;
	GzIntensity rz,gz,bz,az;
	GzDepth zz;
	if(GzGetDisplayParams(display,&xres,&yres) != GZ_SUCCESS)
	{
		return GZ_FAILURE;
	}

	for(j=0;j<yres;j++)
	{
		for(i=0;i<xres;i++)
		{

			if(GzGetDisplay(display,i,j,&rz,&gz,&bz,&az,&zz) != GZ_SUCCESS)
			{
				return GZ_FAILURE;
			}

			r = (unsigned char)((unsigned short)rz >> 4);
		
			b = (unsigned char)((unsigned short)bz >> 4);
			
			g = (unsigned char)((unsigned short)gz >> 4);
			
			framebuffer[ctr++] = b;
			framebuffer[ctr++] = g;
			framebuffer[ctr++] = r;
		}
	}

	return GZ_SUCCESS;
}

