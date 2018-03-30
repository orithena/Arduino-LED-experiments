
/*** Base field functions ***/

inline double sines3D(double x, double y) {
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

inline double halfsines3D(double x, double y) {
  double r = (cos(x) * sin(y));
  return (r > 0.0 ? r : 0.0);
}

inline double ringsines3D(double x, double y) {
  double r = (cos(x) * sin(y));
  return (r > 0.0 && r < 0.666 ? r*1.5 : 0.0);
}

inline double sinecircle3D(double x, double y) {
  return (cos(x) * sin(y) * cos(sqrt((x*x) + (y*y))));
}

/*** Matrix-size-dependent scale factors ***/

const double sfx = kMatrixWidth/4.0;
const double sfy = kMatrixHeight/4.0;

/*** pattern run variables, used by all sinematrix patterns ***/

// primary matrix coefficient run variables
double pangle = 0;      // "proto-angle"
double angle = 0;       // rotation angle
double sx = 0;          // scale factor x
double sy = 0;          // scale factor y
double tx = 0;          // translation x
double ty = 0;          // translation y
double cx = 0;          // center offset x (used for rcx)
double cy = 0;          // center offset y (used for rcy)
double rcx = 0;         // rotation center offset x
double rcy = 0;         // rotation center offset y

// secondary set of matrix coefficient run variables (used for "shadow overlay" in sinematrix2)
double pangle2 = 0;      // "proto-angle"
double angle2 = 0;      // angle 
double sx2 = 0;         // scale factor x
double sy2 = 0;         // scale factor y
double tx2 = 0;         // translation x
double ty2 = 0;         // translation y

double cx2 = 0;          // center offset x (used for rcx)
double cy2 = 0;          // center offset y (used for rcy)
double rcx2 = 0;         // rotation center offset x
double rcy2 = 0;         // rotation center offset y

double pangle3 = 0;      // "proto-angle"
double angle3 = 0;       // rotation angle
double sx3 = 0;          // scale factor x
double sy3 = 0;          // scale factor y
double tx3 = 0;         // translation x
double ty3 = 0;         // translation y
double cx3 = 0;          // center offset x (used for rcx)
double cy3 = 0;          // center offset y (used for rcy)
double rcx3 = 0;         // rotation center offset x
double rcy3 = 0;         // rotation center offset y

double basecol = 0;     // base color offset to start from for each frame

/*** sinematrix pattern functions ***
 *  
 * Each of these functions are structured the same:
 *  1. increment run variables by a small amount, each by a different amount, modulo their repetition value.
 *      Do play around with the increments, I just recommend using fractions of different prime numbers
 *      to avoid pattern repetition. Increasing the increments normally speeds up the animation.
 *      (In case they are fed into sin/cos, the repetition value would be PI. If they go 
 *      into hue, it would be 1.0 or 256, depending on the implementation)
 *  2. construct matrices using the run variables as coefficients.
 *  3. For each pixel, interpret their x and y index as a Vector and multiply that with the 
 *      constructed matrices, and add the translation vectors. 
 *      Using a different operation order, you may get different results!
 *      
 * In summary, you may think of these pattern functions as a camera arm that's mounted in the origin
 * of a static curve in 3-dimensional space (which is defined by the functions sines3D and sinecircle3D),
 * where the z axis is mapped to the hue in HSV color space. Then, by changing the coefficients of the 
 * matrices, that camera arm moves the camera seemingly randomly around, pointing it to different areas 
 * of the pattern while the camera zooms in and out and rotates... 
 * 
 * Feel free to insert more alternating rotation matrices and translation vectors ^^
 */

void sinematrix() {
  angle = addmodpi( angle, 0.019 );
  sx = addmodpi( sx, 0.017);
  sy = addmodpi( sy, 0.013);
  tx = addmodpi( tx, 0.023 );
  ty = addmodpi( ty, 0.029 );
  basecol = addmod( basecol, 1.0, 0.01 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix scale = {
    .a11 = sin(sx)/2.0,
    .a12 = 0,
    .a21 = 0,
    .a22 = sin(sy)/2.0
  };
  Vector translate = {
    .x1 = sin(tx),
    .x2 = sin(ty)
  };
  
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(scale, rotate), { .x1 = x-7, .x2 = y-7 } ), translate);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines3D(c.x1, c.x2))*255, 255, 255);
    }
  }
} 

void sinematrix2() {
  pangle = addmodpi( pangle, 0.0133 + (angle/256) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00173 );
  sy = addmodpi( sy, 0.00137 );
  tx = addmodpi( tx, 0.00239 );
  ty = addmodpi( ty, 0.00293 );
  cx = addmodpi( cx, 0.00197 );
  cy = addmodpi( cy, 0.00227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  angle2 = addmodpi( angle2, 0.0029 );
  sx2 = addmodpi( sx2, 0.0041);
  sy2 = addmodpi( sy2, 0.0031);
  tx2 = addmodpi( tx2, 0.0011 );
  ty2 = addmodpi( ty2, 0.0023 );
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix scale = {
    .a11 = sin(sx)/4.0 + 0.25,
    .a12 = 0,
    .a21 = 0,
    .a22 = cos(sy)/4.0 + 0.25
  };
  Vector translate = {
    .x1 = (-kMatrixWidth/2) + (sin(tx) * kMatrixWidth) + rcx,
    .x2 = (-kMatrixHeight/2) + (sin(ty) * kMatrixHeight) + rcy
  };

  Matrix rotate2 = {
    .a11 = cos(angle2),
    .a12 = -sin(angle2),
    .a21 = sin(angle2),
    .a22 = cos(angle2)
  };
  Matrix scale2 = {
    .a11 = sin(sx2)/2.0,
    .a12 = 0,
    .a21 = 0,
    .a22 = sin(sy2)/2.0
  };
  Vector translate2 = {
    .x1 = sin(tx2),
    .x2 = sin(ty2)
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, scale), { .x1 = x, .x2 = y } ), translate);
      Vector c2 = add(multiply( multiply(scale2, rotate2), { .x1 = x, .x2 = y } ), translate2);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines3D(c.x1, c.x2))*255, 255, 31+(sines3D(c2.x1-10, c2.x2-10)*224));
    }
  }
}

void sinematrix3() {
  pangle = addmodpi( pangle, 0.0133 + (angle/256) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00673 );
  sy = addmodpi( sy, 0.00437 );
  tx = addmodpi( tx, 0.00239 );
  ty = addmodpi( ty, 0.00293 );
  cx = addmodpi( cx, 0.00197 );
  cy = addmodpi( cy, 0.00227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix scale = {
    .a11 = sin(sx)/4.0 + 0.15,
    .a12 = 0,
    .a21 = 0,
    .a22 = cos(sy)/4.0 + 0.15
  };
  Vector translate = {
    .x1 = sin(tx) * kMatrixWidth,
    .x2 = sin(ty) * kMatrixHeight
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, scale), { .x1 = x-rcx, .x2 = y-rcy } ), translate);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sinecircle3D(c.x1, c.x2))*255, 255, 255);
    }
  }
}

void sinematrix_rgb() {
  pangle = addmodpi( pangle, 0.00133 + (angle/256) );
  angle = cos(pangle) * PI;
  pangle2 = addmodpi( pangle2, 0.00323 + (angle2/256) );
  angle2 = cos(pangle2) * PI;
  pangle3 = addmodpi( pangle3, 0.00613 + (angle3/256) );
  angle3 = cos(pangle3) * PI;
  sx = addmodpi( sx, 0.00673 );
  sy = addmodpi( sy, 0.00437 );
  sx2 = addmodpi( sx2, 0.00973 );
  sy2 = addmodpi( sy2, 0.01037 );
  sx3 = addmodpi( sx3, 0.0273 );
  sy3 = addmodpi( sy3, 0.0327 );
  tx = addmodpi( tx, 0.00239 );
  ty = addmodpi( ty, 0.00293 );
  tx2 = addmodpi( tx2, 0.00439 );
  ty2 = addmodpi( ty2, 0.00193 );
  tx3 = addmodpi( tx3, 0.00639 );
  ty3 = addmodpi( ty3, 0.00793 );
  cx = addmodpi( cx, 0.00347 );
  cy = addmodpi( cy, 0.00437 );
  rcx = (kMatrixWidth/2) + (sin(cx2) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy2) * kMatrixHeight);
  cx2 = addmodpi( cx2, 0.00697 );
  cy2 = addmodpi( cy2, 0.00727 );
  rcx2 = (kMatrixWidth/2) + (sin(cx2) * kMatrixWidth);
  rcy2 = (kMatrixHeight/2) + (sin(cy2) * kMatrixHeight);
  cx3 = addmodpi( cx3, 0.00197 );
  cy3 = addmodpi( cy3, 0.00227 );
  rcx3 = (kMatrixWidth/2) + (sin(cx3) * kMatrixWidth);
  rcy3 = (kMatrixHeight/2) + (sin(cy3) * kMatrixHeight);
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotater = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix rotateg = {
    .a11 = cos(angle2),
    .a12 = -sin(angle2),
    .a21 = sin(angle2),
    .a22 = cos(angle2)
  };
  Matrix rotateb = {
    .a11 = cos(angle3),
    .a12 = -sin(angle3),
    .a21 = sin(angle3),
    .a22 = cos(angle3)
  };
  Matrix scaler = {
    .a11 = (sin(sx) + 1.0)/sfx,
    .a12 = 0,
    .a21 = 0,
    .a22 = (cos(sy) + 1.0)/sfy
  };
  Matrix scaleg = {
    .a11 = (sin(sx2) + 1.0)/sfx,
    .a12 = 0,
    .a21 = 0,
    .a22 = (cos(sy2) + 1.0)/sfy
  };
  Matrix scaleb = {
    .a11 = (sin(sx3) + 1.0)/sfx,
    .a12 = 0,
    .a21 = 0,
    .a22 = (cos(sy3) + 1.0)/sfy
  };
  Vector translater = {
    .x1 = sin(tx) * kMatrixWidth,
    .x2 = sin(ty) * kMatrixHeight
  };
  Vector translateg = {
    .x1 = sin(tx2) * kMatrixWidth,
    .x2 = sin(ty2) * kMatrixHeight
  };
  Vector translateb = {
    .x1 = sin(tx3) * kMatrixWidth,
    .x2 = sin(ty3) * kMatrixHeight
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector cr = add(multiply( multiply(rotater, scaler), { .x1 = x-rcx,  .x2 = y-rcy  } ), translater);
      Vector cg = add(multiply( multiply(rotateg, scaleg), { .x1 = x-rcx2, .x2 = y-rcy2 } ), translateg);
      Vector cb = add(multiply( multiply(rotateb, scaleb), { .x1 = x-rcx3, .x2 = y-rcy3 } ), translateb);
      leds[(y*kMatrixWidth) + x] = CRGB(
        (halfsines3D(cr.x1, cr.x2))*255, 
        (halfsines3D(cg.x1, cg.x2))*255, 
        (halfsines3D(cb.x1, cb.x2))*255
      );
    }
  }
}

