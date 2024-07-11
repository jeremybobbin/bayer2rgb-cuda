/* shapes

 Corners  Edges  Across    TB      LR
  x o x   o x o   x o x   o x o   o o o
  o o o   x o x   o x o   o o o   x o x
  x o x   o x o   x o x   o x o   o o o  */

#define CENTER         (     bayer[i]                                                                          )
#define CORNERS        (med4(bayer[(i-1)-w],  bayer[(i+1)-w],  bayer[(i-1)+w],  bayer[(i+1)+w]           )     )
#define EDGES          (med4(bayer[i-w],      bayer[i-1],      bayer[i+1],      bayer[i+w]               )     )
#define ACROSS         (med5(bayer[(i-1)-w],  bayer[(i+1)-w],  bayer[(i-1)+w],  bayer[(i+1)+w],  bayer[i])     )
#define TOP_AND_BOTTOM (    (bayer[i-w]     + bayer[i+w]                                                 ) >> 1)
#define LEFT_AND_RIGHT (    (bayer[i-1]     + bayer[i+1]                                                 ) >> 1)

#define   RED(shape)  rgb[j+0] = shape
#define GREEN(shape)  rgb[j+1] = shape
#define  BLUE(shape)  rgb[j+2] = shape

__device__ unsigned char med4(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
	unsigned char l1, l2, h1, h2, t;
	if (a < b) {
		l1 = a;
		h1 = b;
	} else {
		l1 = b;
		h1 = a;
	}

	if (c < d) {
		l2 = c;
		h2 = d;
	} else {
		l2 = d;
		h2 = c;
	}

	if (h2 < h1) {
		t = h2;
		h2 = h1;
		h1 = t;
	}

	if (l2 < l1) {
		t = l2;
		l2 = l1;
		l1 = t;
	}

	return (unsigned char)(((unsigned short)l2 + (unsigned short)h1) >> 1);
}

__device__ unsigned char med5(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e) {
	unsigned char l1, l2, h1, h2, t;
	if (a < b) {
		l1 = a;
		h1 = b;
	} else {
		l1 = b;
		h1 = a;
	}

	if (c < d) {
		l2 = c;
		h2 = d;
	} else {
		l2 = d;
		h2 = c;
	}

	if (h2 < h1) {
		t = h2;
		h2 = h1;
		h1 = t;
	}

	if (l2 < l1) {
		t = l2;
		l2 = l1;
		l1 = t;
	}

	if (e <= l2) {
		// e  l1 l2 h1 h2
		// l1  e l2 h1 h2
		// l1 l2  e h1 h2
		return e;
	}

	if (e >= h1) {
		// l1 l2 e  h1 h2
		// l1 l2 h1 e  h2
		// l1 l2 h1 h2 e
		return e;
	}

	return e;
}

extern "C" __global__ void debayer(char *buf, int w, int h) {
	unsigned char *bayer = (unsigned char*)buf;
	unsigned char *rgb   = (unsigned char*)&buf[w*h];
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;
	int i = ((y*w)+x); // offset into bayer
	int j = i*3;       // offset into rgb

	// skip edges
	if (y < 1 || y >= (h-1) || x < 1 || x >= (w-1))
		return;

	if ((1 & x & y) == 1) {
		RED(CORNERS); GREEN(EDGES); BLUE(CENTER);
	} else if ((x & 1) == 1 && (y & 1) == 0) {
		RED(LEFT_AND_RIGHT); GREEN(ACROSS); BLUE(TOP_AND_BOTTOM);
	} else if (/*x % 2 == 0 &&*/ (y & 1) == 1) {
		RED(TOP_AND_BOTTOM); GREEN(ACROSS); BLUE(LEFT_AND_RIGHT);
	} else /*if (x % 2 == 0 && y % 2 == 0) */ {
		RED(CENTER); GREEN(EDGES); BLUE(CORNERS);
	}
}
