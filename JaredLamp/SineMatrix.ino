#define ROWS 1
#define COLS 60

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
  x = fmodf(x + delta, mod);
  x += x<0 ? mod : 0;
  return x;
}

inline double addmodpi(double x, double delta) {
  return addmod(x, 2*PI, delta);
}

double fx = 1.0/(COLS/PI);
double fy = 1.0/(ROWS/PI);

double pangle = random(4096) / (4096.0/PI);
double angle = 0;
double sx = random(4096) / (4096.0/PI);
double sy = random(4096) / (4096.0/PI);
double tx = random(4096) / (4096.0/PI);
double ty = random(4096) / (4096.0/PI);
double cx = random(4096) / (4096.0/PI);
double cy = random(4096) / (4096.0/PI);
double rcx = 0;
double rcy = 0;
double basecol = random(4096) / 4096.0;
//const double f = 0.33;

int sinematrix3() {
  EVERY_N_MILLISECONDS( 20 ) {
  double f = 0.05;
  pangle = addmodpi( pangle, (0.0133 + (angle/256)) * (0.2+f) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.000673 * f );
  sy = addmodpi( sy, 0.000437 * f );
  tx = addmodpi( tx, 0.000539 * f );
  ty = addmodpi( ty, 0.000493 * f );
  cx = addmodpi( cx, 0.000571 * f );
  cy = addmodpi( cy, 0.000679 * f );
  rcx = (COLS/2) + (sin(cx) * COLS);
  rcy = (ROWS/2) + (sin(cy) * ROWS);
  basecol = addmod( basecol, 1.0, 0.00097 * f );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix zoom = {
    .a11 = sin(sx)/16.0 + 0.05,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
    .a22 = cos(sy)/16.0 + 0.05
  };
  Vector translate = {
    .x1 = sin(tx) * COLS,
    .x2 = sin(ty) * ROWS
  };

  for( int x = 0; x < COLS; x++ ) {
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x-rcx, .x2 = -rcy } ), translate);
      leds[x] = CHSV((basecol+basefield(c.x1, c.x2))*255, 255, 255);
  }
  return 1;
  }
  return 0;
}
