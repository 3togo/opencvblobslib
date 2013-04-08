/************************************************************************
  			Blob.cpp
  			
- FUNCIONALITAT: Implementaci� de la classe CBlob
- AUTOR: Inspecta S.L.
MODIFICACIONS (Modificaci�, Autor, Data):

 
FUNCTIONALITY: Implementation of the CBlob class and some helper classes to perform
			   some calculations on it
AUTHOR: Inspecta S.L.
MODIFICATIONS (Modification, Author, Date):

**************************************************************************/


#include "blob.h"


CBlob::CBlob()
{
	m_area = m_perimeter = -1;
	m_externPerimeter = m_meanGray = m_stdDevGray = -1;
	m_boundingBox.width = -1;
	m_ellipse.size.width = -1;
	m_storage = NULL;
	m_id = -1;
}
CBlob::CBlob( t_labelType id, CvPoint startPoint, CvSize originalImageSize )
{
	m_id = id;
	m_area = m_perimeter = -1;
	m_externPerimeter = m_meanGray = m_stdDevGray = -1;
	m_boundingBox.width = -1;
	m_ellipse.size.width = -1;
	m_storage = cvCreateMemStorage();
	m_externalContour = CBlobContour(startPoint, m_storage);
	lastStartingPoint = startPoint;
	m_originalImageSize = originalImageSize;
}
//! Copy constructor
CBlob::CBlob( const CBlob &src )
{
	m_storage = NULL;
	*this = src;
}

CBlob::CBlob( const CBlob *src )
{
	if (src != NULL )
	{
		m_storage = NULL;
		*this = *src;
	}
}

CBlob& CBlob::operator=(const CBlob &src )
{
	if( this != &src )
	{
		m_id = src.m_id;
		m_area = src.m_area;
		m_perimeter = src.m_perimeter;
		m_externPerimeter = src.m_externPerimeter;
		m_meanGray = src.m_meanGray;
		m_stdDevGray = src.m_stdDevGray;
		m_boundingBox = src.m_boundingBox;
		m_ellipse = src.m_ellipse;
		m_originalImageSize = src.m_originalImageSize;
		lastStartingPoint = src.lastStartingPoint;
		
		// clear all current blob contours
		ClearContours();
		
		if( m_storage )
			cvReleaseMemStorage( &m_storage );

		m_storage = cvCreateMemStorage();

		m_externalContour = CBlobContour(src.m_externalContour.GetStartPoint(), m_storage );
		if( src.m_externalContour.m_contour )
			m_externalContour.m_contour = cvCloneSeq( src.m_externalContour.m_contour, m_storage);
		m_internalContours.clear();

		// copy all internal contours
		if( src.m_internalContours.size() )
		{
			m_internalContours = t_contourList( src.m_internalContours.size() );
			t_contourList::const_iterator itSrc;
			t_contourList::iterator it;

			itSrc = src.m_internalContours.begin();
			it = m_internalContours.begin();

			while (itSrc != src.m_internalContours.end())
			{
				*it = CBlobContour((*itSrc).GetStartPoint(), m_storage);
				if( (*itSrc).m_contour )
					(*it).m_contour = cvCloneSeq( (*itSrc).m_contour, m_storage);

				it++;
				itSrc++;
			}
		}
	}

	return *this;
}

CBlob::~CBlob()
{
	ClearContours();
	
	if( m_storage )
		cvReleaseMemStorage( &m_storage );
}

void CBlob::ClearContours()
{
	t_contourList::iterator it;

	it = m_internalContours.begin();

	while (it != m_internalContours.end())
	{
		(*it).ResetChainCode();
		it++;
	}	
	m_internalContours.clear();

	m_externalContour.ResetChainCode();
		
}
void CBlob::AddInternalContour( const CBlobContour &newContour )
{
	m_internalContours.push_back(newContour);
}

//! Indica si el blob est� buit ( no t� cap info associada )
//! Shows if the blob has associated information
bool CBlob::IsEmpty()
{
	return GetExternalContour()->m_contour == NULL;
}

/**
- FUNCI�: Area
- FUNCIONALITAT: Get blob area, ie. external contour area minus internal contours area
- PAR�METRES:
	- 
- RESULTAT:
	- 
- RESTRICCIONS:
	- 
- AUTOR: rborras
- DATA DE CREACI�: 2008/04/30
- MODIFICACI�: Data. Autor. Descripci�.
*/
double CBlob::Area()
{
	double area;
	t_contourList::iterator itContour; 

	area = m_externalContour.GetArea();

	itContour = m_internalContours.begin();
	
	while (itContour != m_internalContours.end() )
	{
		area -= (*itContour).GetArea();
		itContour++;
	}
	return area;
}

/**
- FUNCI�: Perimeter
- FUNCIONALITAT: Get blob perimeter, ie. sum of the lenght of all the contours
- PAR�METRES:
	- 
- RESULTAT:
	- 
- RESTRICCIONS:
	- 
- AUTOR: rborras
- DATA DE CREACI�: 2008/04/30
- MODIFICACI�: Data. Autor. Descripci�.
*/
double CBlob::Perimeter()
{
	double perimeter;
	t_contourList::iterator itContour; 

	perimeter = m_externalContour.GetPerimeter();

	itContour = m_internalContours.begin();
	
	while (itContour != m_internalContours.end() )
	{
		perimeter += (*itContour).GetPerimeter();
		itContour++;
	}
	return perimeter;

}

/**
- FUNCI�: Exterior
- FUNCIONALITAT: Return true for extern blobs
- PAR�METRES:
	- xBorder: true to consider blobs touching horizontal borders as extern
	- yBorder: true to consider blobs touching vertical borders as extern
- RESULTAT:
	- 
- RESTRICCIONS:
	- 
- AUTOR: rborras
- DATA DE CREACI�: 2008/05/06
- MODIFICACI�: Data. Autor. Descripci�.
*/
int	CBlob::Exterior(IplImage *mask, bool xBorder /* = true */, bool yBorder /* = true */)
{
	if (ExternPerimeter(mask, xBorder, yBorder ) > 0 )
	{
		return 1;
	}
	
	return 0;	 
}
int	CBlob::Exterior(Mat mask, bool xBorder /* = true */, bool yBorder /* = true */)
{
	return Exterior(&(IplImage) mask, xBorder, yBorder);	 
}
/**
- FUNCI�: ExternPerimeter
- FUNCIONALITAT: Get extern perimeter (perimeter touching image borders)
- PAR�METRES:
	- maskImage: if != NULL, counts maskImage black pixels as external pixels and contour points touching
				 them are counted as external contour points.
	- xBorder: true to consider blobs touching horizontal borders as extern
	- yBorder: true to consider blobs touching vertical borders as extern
- RESULTAT:
	- 
- RESTRICCIONS:
	- 
- AUTOR: rborras
- DATA DE CREACI�: 2008/05/05
- MODIFICACI�: Data. Autor. Descripci�.
- NOTA: If CBlobContour::GetContourPoints aproximates contours with a method different that NONE,
		this function will not give correct results
*/
double CBlob::ExternPerimeter( IplImage *maskImage, bool xBorder /* = true */, bool yBorder /* = true */)
{
	t_PointList externContour, externalPoints;
	CvSeqReader reader;
	CvSeqWriter writer;
	CvPoint actualPoint, previousPoint;
	bool find = false;
	int i,j;
	int delta = 0;
	
	// it is calculated?
	if( m_externPerimeter != -1 )
	{
		return m_externPerimeter;
	}

	// get contour pixels
	externContour = m_externalContour.GetContourPoints();

	m_externPerimeter = 0;

	// there are contour pixels?
	if( externContour == NULL )
	{
		return m_externPerimeter;
	}

	cvStartReadSeq( externContour, &reader);

	// create a sequence with the external points of the blob
	externalPoints = cvCreateSeq( externContour->flags, externContour->header_size, externContour->elem_size, 
								  m_storage );
	cvStartAppendToSeq( externalPoints, &writer );
	previousPoint.x = -1;

	// which contour pixels touch border?
	for( j=0; j< externContour->total; j++)
	{
		CV_READ_SEQ_ELEM( actualPoint, reader);

		find = false;

		// pixel is touching border?
		if ( xBorder & ((actualPoint.x == 0) || (actualPoint.x == m_originalImageSize.width - 1 )) ||
			 yBorder & ((actualPoint.y == 0) || (actualPoint.y == m_originalImageSize.height - 1 )))
		{
			find = true;
		}
		else
		{
			if( maskImage != NULL )
			{
				// verify if some of 8-connected neighbours is black in mask
				char *pMask;
				
				pMask = (maskImage->imageData + actualPoint.x - 1 + (actualPoint.y - 1) * maskImage->widthStep);
				
				for ( i = 0; i < 3; i++, pMask++ )
				{
					if(*pMask == 0 && !find ) 
					{
						find = true;
						break;
					}						
				}
				
				if(!find)
				{
					pMask = (maskImage->imageData + actualPoint.x - 1 + (actualPoint.y ) * maskImage->widthStep);
				
					for ( i = 0; i < 3; i++, pMask++ )
					{
						if(*pMask == 0 && !find ) 
						{
							find = true;
							break;
						}
					}
				}
			
				if(!find)
				{
					pMask = (maskImage->imageData + actualPoint.x - 1 + (actualPoint.y + 1) * maskImage->widthStep);

					for ( i = 0; i < 3; i++, pMask++ )
					{
						if(*pMask == 0 && !find ) 
						{
							find = true;
							break;
						}
					}
				}
			}
		}

		if( find )
		{
			if( previousPoint.x > 0 )
				delta = abs(previousPoint.x - actualPoint.x) + abs(previousPoint.y - actualPoint.y);

			// calculate separately each external contour segment 
			if( delta > 2 )
			{
				cvEndWriteSeq( &writer );
				m_externPerimeter += cvArcLength( externalPoints, CV_WHOLE_SEQ, 0 );
				
				cvClearSeq( externalPoints );
				cvStartAppendToSeq( externalPoints, &writer );
				delta = 0;
				previousPoint.x = -1;
			}

			CV_WRITE_SEQ_ELEM( actualPoint, writer );
			previousPoint = actualPoint;
		}
		
	}

	cvEndWriteSeq( &writer );

	m_externPerimeter += cvArcLength( externalPoints, CV_WHOLE_SEQ, 0 );

	cvClearSeq( externalPoints );

	// divide by two because external points have one side inside the blob and the other outside
	// Perimeter of external points counts both sides, so it must be divided
	m_externPerimeter /= 2.0;
	
	return m_externPerimeter;
}
double CBlob::ExternPerimeter( Mat maskImage, bool xBorder /* = true */, bool yBorder /* = true */){
	return ExternPerimeter( &(IplImage) maskImage, xBorder /* = true */, yBorder /* = true */);
}
//! Compute blob's moment (p,q up to MAX_CALCULATED_MOMENTS)
double CBlob::Moment(int p, int q)
{
	double moment;
	t_contourList::iterator itContour; 

	moment = m_externalContour.GetMoment(p,q);

	itContour = m_internalContours.begin();
	
	while (itContour != m_internalContours.end() )
	{
		moment -= (*itContour).GetMoment(p,q);
		itContour++;
	}
	return moment;
}

/**
- FUNCI�: Mean
- FUNCIONALITAT: Get blob mean color in input image
- PAR�METRES:
	- image: image from gray color are extracted
- RESULTAT:
	- 
- RESTRICCIONS:
	- 
- AUTOR: rborras
- DATA DE CREACI�: 2008/05/06
- MODIFICACI�: Data. Autor. Descripci�.
*/
double CBlob::Mean( IplImage *image )
{
	// it is calculated?
/*	if( m_meanGray != -1 )
	{
		return m_meanGray;
	}
*/	
	// Create a mask with same size as blob bounding box
	IplImage *mask;
	CvScalar mean, std;
	CvPoint offset;

	GetBoundingBox();
	
	if (m_boundingBox.height == 0 ||m_boundingBox.width == 0 || !CV_IS_IMAGE( image ))
	{
		m_meanGray = 0;
		return m_meanGray;
	}

	// apply ROI and mask to input image to compute mean gray and standard deviation
	mask = cvCreateImage( cvSize(m_boundingBox.width, m_boundingBox.height), IPL_DEPTH_8U, 1);
	cvSetZero(mask);

	offset.x = -m_boundingBox.x;
	offset.y = -m_boundingBox.y;

	// draw contours on mask
	cvDrawContours( mask, m_externalContour.GetContourPoints(), CV_RGB(255,255,255), CV_RGB(255,255,255),0, CV_FILLED, 8,
					offset );

	// draw internal contours
	t_contourList::iterator it = m_internalContours.begin();
	while(it != m_internalContours.end() )
	{
		cvDrawContours( mask, (*it).GetContourPoints(), CV_RGB(0,0,0), CV_RGB(0,0,0),0, CV_FILLED, 8,
					offset );
		it++;
	}

	cvSetImageROI( image, m_boundingBox );
	cvAvgSdv( image, &mean, &std, mask );
	
	m_meanGray = mean.val[0];
	m_stdDevGray = std.val[0];

	cvReleaseImage( &mask );
	cvResetImageROI( image );

	return m_meanGray;
}
double CBlob::Mean(Mat image ){
	return Mean(&(IplImage)image);
}
double CBlob::StdDev( IplImage *image )
{
	// it is calculated?
/*	if( m_stdDevGray != -1 )
	{
		return m_stdDevGray;
	}
*/
	// call mean calculation (where also standard deviation is calculated)
	Mean( image );

	return m_stdDevGray;
}
double CBlob::StdDev(Mat image){
	return StdDev(&(IplImage)image);
}
/**
- FUNCI�: GetBoundingBox
- FUNCIONALITAT: Get bounding box (without rotation) of a blob
- PAR�METRES:
	- 
- RESULTAT:
	- 
- RESTRICCIONS:
	- 
- AUTOR: rborras
- DATA DE CREACI�: 2008/05/06
- MODIFICACI�: Data. Autor. Descripci�.
*/
CvRect CBlob::GetBoundingBox()
{
	// it is calculated?
	if( m_boundingBox.width != -1 )
	{
		return m_boundingBox;
	}

	t_PointList externContour;
	CvSeqReader reader;
	CvPoint actualPoint;
	
	// get contour pixels
	externContour = m_externalContour.GetContourPoints();
	
	// it is an empty blob?
	if( !externContour )
	{
		m_boundingBox.x = 0;
		m_boundingBox.y = 0;
		m_boundingBox.width = 0;
		m_boundingBox.height = 0;

		return m_boundingBox;
	}

	cvStartReadSeq( externContour, &reader);

	m_boundingBox.x = m_originalImageSize.width;
	m_boundingBox.y = m_originalImageSize.height;
	m_boundingBox.width = 0;
	m_boundingBox.height = 0;

	for( int i=0; i< externContour->total; i++)
	{
		CV_READ_SEQ_ELEM( actualPoint, reader);

		m_boundingBox.x = MIN( actualPoint.x, m_boundingBox.x );
		m_boundingBox.y = MIN( actualPoint.y, m_boundingBox.y );
		
		m_boundingBox.width = MAX( actualPoint.x, m_boundingBox.width );
		m_boundingBox.height = MAX( actualPoint.y, m_boundingBox.height );
	}

	//m_boundingBox.x = max( m_boundingBox.x , 0 );
	//m_boundingBox.y = max( m_boundingBox.y , 0 );

	m_boundingBox.width -= m_boundingBox.x;
	m_boundingBox.height -= m_boundingBox.y;
	
	return m_boundingBox;
}

/**
- FUNCI�: GetEllipse
- FUNCIONALITAT: Calculates bounding ellipse of external contour points
- PAR�METRES:
	- 
- RESULTAT:
	- 
- RESTRICCIONS:
	- 
- AUTOR: rborras
- DATA DE CREACI�: 2008/05/06
- MODIFICACI�: Data. Autor. Descripci�.
- NOTA: Calculation is made using second order moment aproximation
*/
CvBox2D CBlob::GetEllipse()
{
	// it is calculated?
	if( m_ellipse.size.width != -1 )
		return m_ellipse;
	
	double u00,u11,u01,u10,u20,u02, delta, num, den, temp;

	// central moments calculation
	u00 = Moment(0,0);

	// empty blob?
	if ( u00 <= 0 )
	{
		m_ellipse.size.width = 0;
		m_ellipse.size.height = 0;
		m_ellipse.center.x = 0;
		m_ellipse.center.y = 0;
		m_ellipse.angle = 0;
		return m_ellipse;
	}
	u10 = Moment(1,0) / u00;
	u01 = Moment(0,1) / u00;

	u11 = -(Moment(1,1) - Moment(1,0) * Moment(0,1) / u00 ) / u00;
	u20 = (Moment(2,0) - Moment(1,0) * Moment(1,0) / u00 ) / u00;
	u02 = (Moment(0,2) - Moment(0,1) * Moment(0,1) / u00 ) / u00;


	// elipse calculation
	delta = sqrt( 4*u11*u11 + (u20-u02)*(u20-u02) );
	m_ellipse.center.x = u10;
	m_ellipse.center.y = u01;
	
	temp = u20 + u02 + delta;
	if( temp > 0 )
	{
		m_ellipse.size.width = sqrt( 2*(u20 + u02 + delta ));
	}	
	else
	{
		m_ellipse.size.width = 0;
		return m_ellipse;
	}

	temp = u20 + u02 - delta;
	if( temp > 0 )
	{
		m_ellipse.size.height = sqrt( 2*(u20 + u02 - delta ) );
	}
	else
	{
		m_ellipse.size.height = 0;
		return m_ellipse;
	}

	// elipse orientation
	if (u20 > u02)
	{
		num = u02 - u20 + sqrt((u02 - u20)*(u02 - u20) + 4*u11*u11);
		den = 2*u11;
	}
    else
    {
		num = 2*u11;
		den = u20 - u02 + sqrt((u20 - u02)*(u20 - u02) + 4*u11*u11);
    }
	if( num != 0 && den  != 00 )
	{
		m_ellipse.angle = 180.0 + (180.0 / CV_PI) * atan( num / den );
	}
	else
	{
		m_ellipse.angle = 0;
	}
        
	return m_ellipse;

}

/**
- FUNCTION: FillBlob
- FUNCTIONALITY: 
	- Fills the blob with a specified colour
- PARAMETERS:
	- imatge: where to paint
	- color: colour to paint the blob
- RESULT:
	- modifies input image and returns the seed point used to fill the blob
- RESTRICTIONS:
- AUTHOR: Ricard Borr�s
- CREATION DATE: 25-05-2005.
- MODIFICATION: Date. Author. Description.
*/
void CBlob::FillBlob( IplImage *image, CvScalar color, int offsetX /*=0*/, int offsetY /*=0*/) 					  
{
	cvDrawContours( image, m_externalContour.GetContourPoints(), color, color,0, CV_FILLED, 8 );
}
void CBlob::FillBlob( Mat image, CvScalar color, int offsetX /*=0*/, int offsetY /*=0*/){
	FillBlob(&(IplImage)image,color,offsetX,offsetY);
}

/**
- FUNCTION: GetConvexHull
- FUNCTIONALITY: Calculates the convex hull polygon of the blob
- PARAMETERS:
	- dst: where to store the result
- RESULT:
	- true if no error ocurred
- RESTRICTIONS:
- AUTHOR: Ricard Borr�s
- CREATION DATE: 25-05-2005.
- MODIFICATION: Date. Author. Description.
*/
t_PointList CBlob::GetConvexHull()
{
	CvSeq *convexHull = NULL;

	if( m_externalContour.GetContourPoints() )
		convexHull = cvConvexHull2( m_externalContour.GetContourPoints(), m_storage,
					   CV_COUNTER_CLOCKWISE, 1 );

	return convexHull;
}

/**
- FUNCTION: JoinBlob
- FUNCTIONALITY: Add's external contour to current external contour
- PARAMETERS:
	- blob: blob from which extract the added external contour
- RESULT:
	- true if no error ocurred
- RESTRICTIONS: Only external contours are added
- AUTHOR: Ricard Borr�s
- CREATION DATE: 25-05-2005.
- MODIFICATION: Date. Author. Description.
*/
void CBlob::JoinBlob( CBlob *blob )
{
	/* Luca Nardelli & Saverio Murgia
	Freeman Chain Code:	
		321		Values indicate the chain code used to identify next pixel location.
		4-0		If I join 2 blobs I can't just append the 2nd blob chain codes, since they will still start
		567		from the 1st blob start point
	*/

	CvSeqWriter writer;
	CvSeqReader reader;
	t_chainCode chainCode;

	cvStartAppendToSeq( m_externalContour.GetChainCode(), &writer );
	cvStartReadSeq( blob->GetExternalContour()->GetChainCode(), &reader );
	int diffX = blob->m_externalContour.m_startPoint.x - lastStartingPoint.x;
	int diffY = blob->m_externalContour.m_startPoint.y - lastStartingPoint.y;
	if(diffX < 0)
		chainCode = 4;
	else
		chainCode = 0;
	diffX=abs(diffX);
	for(int i=0;i<diffX;i++){
		CV_WRITE_SEQ_ELEM(chainCode,writer);
	}
	if(diffY < 0)
		chainCode = 2;
	else
		chainCode = 6;
	diffY=abs(diffY);
	for(int i=0;i<diffY;i++){
		CV_WRITE_SEQ_ELEM(chainCode,writer);
	}
	//chainCode=6;
	//for(int i=0;i<800;i++)
	//	CV_WRITE_SEQ_ELEM(chainCode,writer);
	for (int i = 0; i < blob->GetExternalContour()->GetChainCode()->total; i++ )
	{
		CV_READ_SEQ_ELEM( chainCode, reader );
		CV_WRITE_SEQ_ELEM( chainCode, writer );
	}
	lastStartingPoint = blob->GetExternalContour()->m_startPoint;
	cvEndWriteSeq( &writer );
	m_externalContour.m_contourPoints = NULL;
	m_boundingBox.width=-1;
}

vector<Point> CBlob::getPointsTouchingBorder( int border )
{
	vector<Point> points;
	CvPoint pt;
	CvSeqReader reader;
	cvStartReadSeq(m_externalContour.GetContourPoints(),&reader);
	for(int i=0;i< m_externalContour.GetContourPoints()->total;i++){
		CV_READ_SEQ_ELEM(pt,reader);
		switch(border){
		case 0:	//Top
			if(pt.y == 0)
				points.push_back(Point(pt));
			break;
		case 1: // Right
			if(pt.x == m_originalImageSize.width-1 )
				points.push_back(Point(pt));
			break;
		case 2: // Bottom
			if(pt.y == m_originalImageSize.height-1 )
				points.push_back(Point(pt));
			break;
		case 4: // Left
			if(pt.x == 0 )
				points.push_back(Point(pt));
			break;
		}
	}
	return points;
}
