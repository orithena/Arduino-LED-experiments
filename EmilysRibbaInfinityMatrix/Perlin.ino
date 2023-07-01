
/*
 Ken Perlins improved noise   -  http://mrl.nyu.edu/~perlin/noise/
 C-port:  http://www.fundza.com/c4serious/noise/perlin/perlin.html
 by Malcolm Kesson;   arduino port by Peter Chiochetti, Sep 2007 :
 -  make permutation constant byte, obsoletes init(), lookup % 256
*/

static const byte p[] = {   151,160,137,91,90, 15,131, 13,201,95,96,
53,194,233, 7,225,140,36,103,30,69,142, 8,99,37,240,21,10,23,190, 6,
148,247,120,234,75, 0,26,197,62,94,252,219,203,117, 35,11,32,57,177,
33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,
48,27,166, 77,146,158,231,83,111,229,122, 60,211,133,230,220,105,92,
41,55,46,245,40,244,102,143,54,65,25,63,161, 1,216,80,73,209,76,132,
187,208, 89, 18,169,200,196,135,130,116,188,159, 86,164,100,109,198,
173,186, 3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,
212,207,206, 59,227, 47,16,58,17,182,189, 28,42,223,183,170,213,119,
248,152,2,44,154,163,70,221,153,101,155,167,43,172, 9,129,22,39,253,
19,98,108,110,79,113,224,232,178,185,112,104,218,246, 97,228,251,34,
242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,
150,254,138,236,205, 93,222,114, 67,29,24, 72,243,141,128,195,78,66,
215,61,156,180
};

float fade(float t){ return t * t * t * (t * (t * 6 - 15) + 10); }
float lerp(float t, float a, float b){ return a + t * (b - a); }
float grad(int hash, float x, float y, float z)
{
int     h = hash & 15;          /* CONVERT LO 4 BITS OF HASH CODE */
float  u = h < 8 ? x : y,      /* INTO 12 GRADIENT DIRECTIONS.   */
          v = h < 4 ? y : h==12||h==14 ? x : z;
return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

#define P(x) p[(x) & 255]

float pnoise(float x, float y, float z)
{
int   X = (int)floor(x) & 255,             /* FIND UNIT CUBE THAT */
      Y = (int)floor(y) & 255,             /* CONTAINS POINT.     */
      Z = (int)floor(z) & 255;
x -= floor(x);                             /* FIND RELATIVE X,Y,Z */
y -= floor(y);                             /* OF POINT IN CUBE.   */
z -= floor(z);
float   u = fade(x),                       /* COMPUTE FADE CURVES */
        v = fade(y),                       /* FOR EACH OF X,Y,Z.  */
        w = fade(z);
int  A = P(X)+Y, 
     AA = P(A)+Z, 
     AB = P(A+1)+Z,                        /* HASH COORDINATES OF */
     B = P(X+1)+Y, 
     BA = P(B)+Z, 
     BB = P(B+1)+Z;                        /* THE 8 CUBE CORNERS, */

return lerp(w,lerp(v,lerp(u, grad(P(AA  ), x, y, z),   /* AND ADD */
                          grad(P(BA  ), x-1, y, z)),   /* BLENDED */
              lerp(u, grad(P(AB  ), x, y-1, z),        /* RESULTS */
                   grad(P(BB  ), x-1, y-1, z))),       /* FROM  8 */
            lerp(v, lerp(u, grad(P(AA+1), x, y, z-1),  /* CORNERS */
                 grad(P(BA+1), x-1, y, z-1)),          /* OF CUBE */
              lerp(u, grad(P(AB+1), x, y-1, z-1),
                   grad(P(BB+1), x-1, y-1, z-1))));
}

double perlin_f(float x, float y, float z) {
  double r = pnoise(x,y,z);
//  if( (r > 0.1 && r < 0.4) || (r > 0.6 && r < 0.9) ) {
  if( (r > 0.05 && r < 0.3) ) {
    return 1.0;
  } else {
    return 0.0;
  }
}

void perlinmatrix() {
  pangle = addmodpi( pangle, 0.00633 + (angle/1024.0) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00673 );
  sy = addmodpi( sy, 0.00437 );
  tx = addmodpi( tx, 0.000239 );
  ty = addmodpi( ty, 0.000293 );
  cx = addmodpi( cx, 0.000197 );
  cy = addmodpi( cy, 0.000227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  angle2 = addmodpi( angle2, 0.0029 );
  sx2 = addmodpi( sx2, 0.0041);
  sy2 = addmodpi( sy2, 0.0031);
  tx2 = addmodpi( tx2, 0.0011 );
  ty2 = addmodpi( ty2, 0.0023 );
  //basecol = addmod( basecol, 1.0, 0.007 );
  basecol += 0.007;
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix zoom = {
    .a11 = sin(sx)/8.0 + 0.05,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
    .a22 = cos(sy)/8.0 + 0.05
  };
  Vector translate = {
    .x1 = sin(tx) * kMatrixWidth,
    .x2 = sin(ty) * kMatrixHeight
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x-rcx, .x2 = y-rcy } ), translate);
      leds(x,y) = CHSV((basecol + pnoise(c.x1, c.x2, pangle))*255, 255, perlin_f(c.x1, c.x2, 0)*255);
    }
  }
}
