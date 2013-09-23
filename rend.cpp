#include	"stdafx.h"
#include	"stdio.h"
#include	"math.h"
#include	"Gz.h"
#include	"rend.h"
#include	"float.h"
#include	"math.h"


int GzNewRender(GzRender **render, GzDisplay *display)
{
/* 
- malloc a renderer struct
- span interpolator needs pointer to display for pixel writes
*/
	*render = (GzRender*)malloc(sizeof(GzRender));
	(*render)->display = display;
	return GZ_SUCCESS;
}


int GzFreeRender(GzRender *render)
{
/* 
-free all renderer resources
*/
	if(render != NULL)
	{
		free(render);
	}
	return GZ_SUCCESS;
}


int GzBeginRender(GzRender	*render)
{
/* 
- set up for start of each frame - init frame buffer
*/
	GzInitDisplay(render->display);
	return GZ_SUCCESS;
}


int GzPutAttribute(GzRender	*render, int numAttributes, GzToken	*nameList, 
	GzPointer *valueList) /* void** valuelist */
{
/*
- set renderer attribute states (e.g.: GZ_RGB_COLOR default color)
- later set shaders, interpolaters, texture maps, and lights
*/
	for(int attribute=0;attribute<numAttributes;attribute++)
	{
		switch(nameList[attribute])
		{
		case GZ_RGB_COLOR:
			memcpy(render->flatcolor,valueList[attribute],sizeof(render->flatcolor));
			break;
		default:
			return GZ_FAILURE;
		}
	}
	return GZ_SUCCESS;
}

int findEdgeEquationParameters(GzCoord v1,GzCoord v2, float* a,float* b,float* c)
{
	*a = v2[Y] - v1[Y];
	*b = -(v2[X] - v1[X]);
	*c = ((v2[X] - v1[X])*v1[Y]) - ((v2[Y] - v1[Y])*v1[X]);
	return GZ_SUCCESS;
}

int sortTriangleVertices(GzCoord unsorted[])
{
	GzCoord temp;
	float a=0,b=0,c=0;

	for(int i=0;i<3;i++)
	{
		for(int j=i+1;j<3;j++)
		{
			if(unsorted[i][Y] > unsorted[j][Y])
			{
				memcpy(temp,unsorted[i],sizeof(GzCoord));
				memcpy(unsorted[i],unsorted[j],sizeof(GzCoord));
				memcpy(unsorted[j],temp,sizeof(GzCoord));
			}
		}
	}

	//X ordering

	findEdgeEquationParameters(unsorted[0],unsorted[2],&a,&b,&c);
	//ax+by+c=0

	float x = -(b*unsorted[1][Y] + c)/a;
	if(x > unsorted[1][X])
	{
		//left edge. Swap second and third
		memcpy(temp,unsorted[1],sizeof(GzCoord));
		memcpy(unsorted[1],unsorted[2],sizeof(GzCoord));
		memcpy(unsorted[2],temp,sizeof(GzCoord));
	}
	//else no need to change. Already clockwise as per Y ordering.
	
	return GZ_SUCCESS;
}

int findTriangleBoundingBox(GzCoord triangle[],float* ulx, float* uly, float* lrx, float* lry)
{
	*uly = min(triangle[0][Y],min(triangle[1][Y],triangle[2][Y]));
	*lry = max(triangle[0][Y],max(triangle[1][Y],triangle[2][Y]));

	*ulx = min(triangle[0][X],min(triangle[1][X],triangle[2][X]));
	*lrx = max(triangle[0][X],max(triangle[1][X],triangle[2][X]));
	
	return GZ_SUCCESS;
}

int isPixelInside(GzCoord triangle[], int pixelX,int pixelY)
{
	float a,b,c;
	bool ret=false;
	float dy=0,dx=0;

	float res1 = ((triangle[1][Y] - triangle[0][Y])*(pixelX - triangle[0][X])) - ((triangle[1][X] - triangle[0][X])*(pixelY-triangle[0][Y]));
	
	float res2 = ((triangle[2][Y] - triangle[1][Y])*(pixelX - triangle[1][X])) - ((triangle[2][X] - triangle[1][X])*(pixelY-triangle[1][Y]));
	
	float res3 = ((triangle[0][Y] - triangle[2][Y])*(pixelX - triangle[2][X])) - ((triangle[0][X] - triangle[2][X])*(pixelY-triangle[2][Y]));
	
	/* Convention: Top edges and Right edges win. As per what I see,
	 * edge 1 can be a top edge, or edge 2 can be a bottom edge.
	 * Also edge 1 is always the right edge. Correct? In which case,
	 * only res1=0 is allowed.
	 */
	
	if(res1<=0 && res2<0 && res3<0)
		return GZ_SUCCESS;

	return GZ_FAILURE;
}

/* NOT part of API - just for general assistance */

short	ctoi(float color)		/* convert float color to GzIntensity short */
{
  return(short)((int)(color * ((1 << 12) - 1)));
}

int GzPutTriangle(GzRender *render, int	numParts, GzToken *nameList,
	GzPointer *valueList) 
/* numParts - how many names and values */
{
/* 
- pass in a triangle description with tokens and values corresponding to
      GZ_NULL_TOKEN:		do nothing - no values
      GZ_POSITION:		3 vert positions in model space
- Invoke the scan converter and return an error code
*/
	GzCoord vertices[3];
	GzCoord edge1[2],edge2[2],edge3[2];
	GzCoord v1,v2,v3;
	GzDepth z=0;
	float a=0,b=0,c=0,d=0;
	float ulx=0,uly=0,lrx=0,lry=0;
	float tempz=0,ux=0,uy=0,uz=0,vx=0,vy=0,vz=0;

	GzIntensity red,green,blue,alpha;
	GzDepth zval=0;

	for(int part=0;part<numParts;part++)
	{
		switch(nameList[part])
		{
		case GZ_POSITION:
			memcpy(vertices,valueList[part],3*sizeof(GzCoord));
			break;
		case GZ_NULL_TOKEN:
		default:
			break;
		}
	}

	sortTriangleVertices(vertices);
	findTriangleBoundingBox(vertices,&ulx,&uly,&lrx,&lry);

	//Find D to interpolate Z
	ux = (vertices[1][X]-vertices[0][X]);
	uy = (vertices[1][Y]-vertices[0][Y]);
	uz = (vertices[1][Z]-vertices[0][Z]);
	
	vx = (vertices[2][X]-vertices[1][X]);
	vy = (vertices[2][Y]-vertices[1][Y]);
	vz = (vertices[2][Z]-vertices[1][Z]);				

	a = (uy*vz)-(uz*vy);
	b = (uz*vx)-(ux*vz);
	c = (ux*vy)-(uy*vx);

	d = -((a*vertices[0][X])+(b*vertices[0][Y])+(c*vertices[0][Z]));

	for(int y=uly;y<lry;y++)
	{
		for(int x=ulx;x<lrx;x++)
		{
			if(isPixelInside(vertices,x,y) == GZ_SUCCESS)
			{
				//Interpolate Z
				z = -((a*x)+(b*y)+d)/c;
				//check in framebuffer if any pixel exists, and if exists, check z value.
				GzGetDisplay(render->display,x,y,&red,&green,&blue,&alpha,&zval);
				{
					//put pixel (x,y) in framebuffer depending on whether the z buffer value is lower or not.
					if(z < zval)
					{
						GzPutDisplay(render->display,x,y,ctoi(render->flatcolor[RED]),ctoi(render->flatcolor[GREEN]),ctoi(render->flatcolor[BLUE]),1,z);
					}
				}
			}
		}
	}

	return GZ_SUCCESS;
}


