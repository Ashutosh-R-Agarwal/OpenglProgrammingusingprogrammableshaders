// THIS HEADER FILE CONTAINS THE DEFINITIONS OF THE FUNCTIONS REQUIRED FOR MATRIX AND VECTOR COMPUTATIONS
// MOSTLY USED TO DEFINE MATRICES AND VECTORS FOR GRAPHICS PROGRAMMING 

#define _USE_MATH_DEFINES  1 // Include constants defined in math.h, linke the constant 'M_PI' is used in the function radians()
#include <math.h>

void myIdentity(float identityMatrix[])
{
	//variable declarations
	int i;

	//code
	for (i = 0; i < 16; i++)
	{
		if (i == 0 || i == 5 || i == 10 || i == 15)
			identityMatrix[i] = 1.0f;
		else
			identityMatrix[i] = 0.0f;
	}
}

void myPerspective(float perspectiveMatrix[], float fovy, float aspect, float n, float f)
{
	//function prototypes
	float radians(float);

	//variable declarations
	float q, A, B, C;
	int i = 0;

	//code
	q = 1.0f / tan(radians(0.5f * fovy));
	A = q / aspect;
	B = (n + f) / (n - f);
	C = (2.0f * n * f) / (n - f);

	for (i = 0; i < 16; i++)
	{
		if (i == 0)
			perspectiveMatrix[i] = A;
		else if (i == 5)
			perspectiveMatrix[i] = q;
		else if (i == 10)
			perspectiveMatrix[i] = B;
		else if (i == 11)
			perspectiveMatrix[i] = -1.0f;
		else if (i == 14)
			perspectiveMatrix[i] = C;
		else
			perspectiveMatrix[i] = 0.0f;
	}
}

void myOrtho(float orthographicMatrix[], float left, float right, float bottom, float top, float n, float f)
{
	//variable declarations
	int i;

	//code
	for (i = 0; i < 16; i++)
	{
		if (i == 0)
			orthographicMatrix[i] = 2.0f / (right - left);
		else if (i == 5)
			orthographicMatrix[i] = 2.0f / (top - bottom);
		else if (i == 10)
			orthographicMatrix[i] = 2.0f / (n - f);
		else if (i == 12)
			orthographicMatrix[i] = (left + right) / (left - right);
		else if (i == 13)
			orthographicMatrix[i] = (bottom + top) / (bottom - top);
		else if (i == 14)
			orthographicMatrix[i] = (n + f) / (f - n);
		else if (i == 15)
			orthographicMatrix[i] = 1.0f;
		else
			orthographicMatrix[i] = 0.0f;
	}
}

void myTranslate(float translationMatrix[], float x, float y, float z)
{
	//variable declarations
	int i;

	//code
	for (i = 0; i < 16; i++)
	{
		if (i == 0 || i == 5 || i == 10 || i == 15)
			translationMatrix[i] = 1.0f;
		else if (i == 12)
			translationMatrix[i] = x;
		else if (i == 13)
			translationMatrix[i] = y;
		else if (i == 14)
			translationMatrix[i] = z;
		else
			translationMatrix[i] = 0.0f;
	}
}

void myRotateTriaxial(float rotationMatrix[], float angle_x, float angle_y, float angle_z)
{
	//function prototype
	void myRotate(float[], float, float, float, float);
	void multiplyMatrices1(float[], float[]);

	//variable declarations
	float rotationMatrix_y[16];
	float rotationMatrix_x[16];

	//code
	myRotate(rotationMatrix, angle_z, 0.0f, 0.0f, 1.0f);
	myRotate(rotationMatrix_y, angle_y, 0.0f, 1.0f, 0.0f);
	myRotate(rotationMatrix_x, angle_x, 1.0f, 0.0f, 0.0f);

	//rotationMatrix_y = rotationMatrix_y * rotationMatrix_x
	multiplyMatrices1(rotationMatrix_x, rotationMatrix_y);

	//rotationMatrix = rotationMatrix * rotationMatrix_y (i.e : rotationMatrix = rotationMatrix * rotationMatrix_y * rotationMatrix_x)
	multiplyMatrices1(rotationMatrix_y, rotationMatrix);
}

void myRotate(float rotationMatrix[], float angle, float x, float y, float z)
{
	//variable declarations
	const float x2 = x * x;
	const float y2 = y * y;
	const float z2 = z * z;
	const float rads = angle * 0.0174532925f;
	const float c = cosf(rads);
	const float s = sinf(rads);
	const float omc = 1.0f - c;
	int i;

	//code
	for (i = 0; i < 16; i++)
	{
		if (i == 0)
			rotationMatrix[i] = x2 * omc + c;
		else if (i == 1)
			rotationMatrix[i] = y * x * omc + z * s;
		else if (i == 2)
			rotationMatrix[i] = x * z * omc - y * s;
		else if (i == 4)
			rotationMatrix[i] = x * y * omc - z * s;
		else if (i == 5)
			rotationMatrix[i] = y2 * omc + c;
		else if (i == 6)
			rotationMatrix[i] = y * z * omc + x * s;
		else if (i == 8)
			rotationMatrix[i] = x * z * omc + y * s;
		else if (i == 9)
			rotationMatrix[i] = y * z * omc - x * s;
		else if (i == 10)
			rotationMatrix[i] = z2 * omc + c;
		else if (i == 15)
			rotationMatrix[i] = 1.0f;
		else
			rotationMatrix[i] = 0.0f;
	}
}

void myScale(float scaleMatrix[], float x, float y, float z)
{
	//variable declarations
	int i;

	//code
	for (i = 0; i < 16; i++)
	{
		if (i == 0)
			scaleMatrix[i] = x;
		else if (i == 5)
			scaleMatrix[i] = y;
		else if (i == 10)
			scaleMatrix[i] = z;
		else if (i == 15)
			scaleMatrix[i] = 1.0f;
		else
			scaleMatrix[i] = 0.0f;
	}
}


//to be used in the case -> matrix2 = matrix1 * matrix2 (eg: modelViewMatrix = modelViewMatrix * rotationMatrix)
//then this function will be called like multiplyMatrices(rotationMatrix, modelViewMatrix);
void multiplyMatrices1(float matrix1[], float matrix2[]) 
{
	//varibale declarations
	int row, col, i, j, k;
	float product = 0.0f;
	float bufferMatrix[16];

	//code
	for (i = 0; i < 16; i++)
	{
		bufferMatrix[i] = matrix2[i];
	}

	row = 0;
	col = 0;
	for (i = 0; i < 16; i++)
	{
		product = 0.0f;
		for (j = row, k = col; j < 4, k < 16; j++, k = k + 4)
		{
			product = product + (matrix1[j] * bufferMatrix[k]);
		}
		matrix2[i] = product;

		col++;
		if (col == 4)
			col = 0;

		if (i == 3 || i == 7 || i == 11)
			row = row + 4;
	}
}

//to be used in the case -> matrix3 = matrix1 * matrix2 (eg: modelViewProjectionMatrix = projectionMatrixMatrix * modelViewMatrix)
//then this function will be called like multiplyMatrices(modelViewProjectionMatrix, modelViewMatrix, projectionMatrix);
void multiplyMatrices2(float productMatrix[], float matrix1[], float matrix2[])
{
	//varibale declarations
	int row, col, i, j, k;
	float product = 0.0f;

	//code
	row = 0;
	col = 0;
	for (i = 0; i < 16; i++)
	{
		for (j = row, k = col; j < 4, k < 16; j++, k = k + 4)
		{
			product = product + (matrix1[j] * matrix2[k]);
		}
		productMatrix[i] = product;
		product = 0.0f;

		col++;
		if (col == 4)
			col = 0;

		if (i == 3 || i == 7 || i == 11)
			row = row + 4;
	}
}

float radians(float angleInDegrees)
{
	//code
	return((float)(angleInDegrees * (M_PI / 180.0f))); //M_PI is the macro for constant pi defined in 'math.h'
}
