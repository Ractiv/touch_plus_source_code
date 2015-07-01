class CWarper
{
public: 
	CWarper(void);
	void setSource(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);
	void setDestination(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);
	void warp(float srcX, float srcY, float &dstX, float &dstY);


private:
	float srcX[4];
	float srcY[4];
	float dstX[4];
	float dstY[4];
	float srcMat[16];
	float dstMat[16];
	float warpMat[16];
	bool   dirty;

	void computeSquareToQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float mat[]);
	void computeQuadToSquare(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float mat[]);
	void multMats(float srcMat[], float dstMat[], float resMat[]);
	void setIdentity(void);
	void warp(float mat[], float srcX, float srcY, float &dstX, float &dstY);
	void computeWarp(void);
};