typedef struct Vector {
  double x1;
  double x2;
} Vector;

typedef struct Matrix {
  double a11;
  double a12;
  double a21;
  double a22;
} Matrix;

struct Matrix multiply(struct Matrix m1, struct Matrix m2) {
  Matrix r = {
      .a11 = m1.a11*m2.a11 + m1.a12*m2.a21,
      .a12 = m1.a11*m2.a12 + m1.a12*m2.a22,
      .a21 = m1.a21*m2.a11 + m1.a22*m2.a21,
      .a22 = m1.a21*m2.a12 + m1.a22*m2.a22
    };
  return r;
};

struct Vector multiply(struct Matrix m, struct Vector v) {
  Vector r = {
      .x1 = (m.a11*v.x1) + (m.a12*v.x2),
      .x2 = (m.a21*v.x1) + (m.a22*v.x2)
    };
  return r;
}

struct Vector add(struct Vector v1, struct Vector v2) {
  Vector r = {
    .x1 = v1.x1 + v2.x2,
    .x2 = v1.x2 + v2.x2
  };
  return r;
}

inline double sines(double x, double y) {
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

inline double basefield(double x, double y) {
  return (cos(x) * sin(y) * cos(sqrt((x*x) + (y*y))));
}

inline double addmod(double x, double mod, double delta) {
  x = x + delta;
  while( x >= mod ) x -= mod;
  while( x <  0.0 ) x += mod;
  return x;
}

inline double addmodpi(double x, double delta) {
  return addmod(x, 2*PI, delta);
}

double fx = 1.0/(kMatrixWidth/PI);
double fy = 1.0/(kMatrixHeight/PI);

double pangle = 0;
double angle = 0;
double sx = 0;
double sy = 0;
double tx = 0;
double ty = 0;
double cx = 0;
double cy = 0;
double rcx = 0;
double rcy = 0;
double angle2 = 0;
double sx2 = 0;
double sy2 = 0;
double tx2 = 0;
double ty2 = 0;
double basecol = 0;

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
  Matrix zoom = {
    .a11 = sin(sx)/4.0 + 0.25,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
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
  Matrix zoom2 = {
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
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x, .x2 = y } ), translate);
      Vector c2 = add(multiply( multiply(zoom2, rotate2), { .x1 = x, .x2 = y } ), translate2);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines(c.x1, c.x2))*255, 255, 31+(sines(c2.x1-10, c2.x2-10)*224));
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
  Matrix zoom = {
    .a11 = sin(sx)/4.0 + 0.15,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
    .a22 = cos(sy)/4.0 + 0.15
  };
  Vector translate = {
    .x1 = sin(tx) * kMatrixWidth,
    .x2 = sin(ty) * kMatrixHeight
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x-rcx, .x2 = y-rcy } ), translate);
      //Vector c2 = add(multiply( multiply(zoom2, rotate2), { .x1 = x, .x2 = y } ), translate2);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+basefield(c.x1, c.x2))*255, 255, 255); //31+(sines(c2.x1-10, c2.x2-10)*224));
    }
  }
}

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
  Matrix zoom = {
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
      Vector c = add(multiply( multiply(zoom, rotate), { .x1 = x, .x2 = y } ), translate);
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines(c.x1-4, c.x2-4))*255, 255, 255);
    }
  }

//  DrawNumberOneFrame(hour(cur), 10, 7);
//  DrawNumberOneFrame(minute(cur), 7, 0);
} 

double calc(double px, double py, double dx, double dy, double ox, double oy) {
  double x = px + (dx*ox);
  double y = py + (dy*oy);
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

double hpx = 0.5, hpy = 0.5, hdx = PI/8, hdy = PI/8;
double spx = 0.5, spy = 0.5, sdx = 0.0, sdy = 0.0;
double vpx = 0.5, vpy = 0.5, vdx = 0.0, vdy = 0.0;

#define SPR 9
#define SPROFF 4
#define HFX 0.007
#define HFY 0.005
#define SFX 0.002
#define SFY 0.003
#define VFX 0.008
#define VFY 0.012

void stuff() {
  //px += (random(10) - 3) / 1000.0;
  //py += (random(10) - 3) / 1000.0;
  hpx += hdx;  hpy += hdy;  hdx += (double)(random(SPR) - SPROFF) * HFX;  hdy += (double)(random(SPR) - SPROFF) * HFY;
  //if( hpx > PI ) hpx -= PI;  if( hpx < -PI ) hpx += PI;  
  if( hdx > PI/4 ) hdx = -PI/4;  if( hdx < -PI/4 ) hdx = PI/4;
  //if( hpy > PI ) hpy -= PI;  if( hpy < -PI ) hpy += PI;  
  if( hdy > PI/4 ) hdy = -PI/4;  if( hdy < -PI/4 ) hdy = PI/4;
  spx += sdx;  spy += sdy;  sdx += (double)(random(SPR) - SPROFF) * SFX;  sdy += (double)(random(SPR) - SPROFF) * SFY;
  //if( spx > PI ) spx = PI;  if( spx < -PI ) spx = -PI;  
  if( sdx > PI/8 ) sdx = -PI/8;  if( sdx < -PI/8 ) sdx = PI/8;
  //if( spy > PI ) spy = PI;  if( spy < -PI ) spy = -PI;  
  if( sdy > PI/8 ) sdy = -PI/8;  if( sdy < -PI/8 ) sdy = PI/8;
  vpx += vdx;  vpy += vdy;  vdx += (double)(random(SPR) - SPROFF) * VFX;  vdy += (double)(random(SPR) - SPROFF) * VFY;
  //if( vpx > PI ) vpx = PI;  if( vpx < -PI ) vpx = -PI;  
  if( vdx > PI/8 ) vdx = -PI/16;  if( vdx < -PI/16 ) vdx = PI/16;
  //if( vpy > PI ) vpy = PI;  if( vpy < -PI ) vpy = -PI;  
  if( vdy > PI/8 ) vdy = -PI/16;  if( vdy < -PI/16 ) vdy = PI/16;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      leds[ (x*kMatrixWidth) + y ] = CHSV(
        calc(hpx,hpy,hdx,hdy,x,y) * 254,
        ((calc(spx,spy,sdx,sdy,x,y) * 0.5) + 0.5) * 254,
        ((calc(vpx,vpy,vdx,vdy,x,y) * 0.75) + 0.25) * 254
      );
    }
  } 
}

void sinefield() {
  float step = (millis() >> 6) & 0x003FFFFF; 
  byte hue = 0;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    hue = step + (37 * sin( ((y*step)/(kMatrixHeight*PI)) * 0.04 ) );
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      hue += 17 * sin(x/(kMatrixWidth*PI));
      leds[ (x*kMatrixHeight) + y ] = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
}


