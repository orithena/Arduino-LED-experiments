/*** Linear Algebra math functions needed for projection ***/

#include <stdarg.h>

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

typedef struct AVector {
  double x1;
  double x2;
  double x3;
} AVector;

typedef struct AMatrix {
  double a11;
  double a12;
  double a13;
  double a21;
  double a22;
  double a23;
  double a31;
  double a32;
  double a33;
} AMatrix;

void printMatrix(struct AMatrix m) {
  Serial.printf("+------------------------------+\n");
  Serial.printf("| %8.4f  %8.4f  %8.4f |\n", m.a11, m.a12, m.a13);
  Serial.printf("| %8.4f  %8.4f  %8.4f |\n", m.a21, m.a22, m.a23);
  Serial.printf("| %8.4f  %8.4f  %8.4f |\n", m.a31, m.a32, m.a33);
  Serial.printf("+------------------------------+\n");
}

void printVector(struct Vector v) {
  Serial.printf("(%8.4f, %8.4f)\n", v.x1, v.x2);
}

struct Matrix multiply(struct Matrix m1, struct Matrix m2) {
  Matrix r = {
      .a11 = m1.a11*m2.a11 + m1.a12*m2.a21,
      .a12 = m1.a11*m2.a12 + m1.a12*m2.a22,
      .a21 = m1.a21*m2.a11 + m1.a22*m2.a21,
      .a22 = m1.a21*m2.a12 + m1.a22*m2.a22
    };
  return r;
};

struct AMatrix multiply(struct AMatrix m1, struct AMatrix m2) {
  AMatrix r = {
      .a11 = m1.a11*m2.a11 + m1.a12*m2.a21 + m1.a13*m2.a31,
      .a12 = m1.a11*m2.a12 + m1.a12*m2.a22 + m1.a13*m2.a32,
      .a13 = m1.a11*m2.a13 + m1.a12*m2.a23 + m1.a13*m2.a33,
      
      .a21 = m1.a21*m2.a11 + m1.a22*m2.a21 + m1.a23*m2.a31,
      .a22 = m1.a21*m2.a12 + m1.a22*m2.a22 + m1.a23*m2.a32,
      .a23 = m1.a21*m2.a13 + m1.a22*m2.a23 + m1.a23*m2.a33,

      .a31 = m1.a31*m2.a11 + m1.a32*m2.a21 + m1.a33*m2.a31,
      .a32 = m1.a31*m2.a12 + m1.a32*m2.a22 + m1.a33*m2.a32,
      .a33 = m1.a31*m2.a13 + m1.a32*m2.a23 + m1.a33*m2.a33
    };
  return r;
};

struct AMatrix identity() {
  AMatrix r = {
    .a11 = 1,
    .a12 = 0,
    .a13 = 0,
    .a21 = 0,
    .a22 = 1,
    .a23 = 0,
    .a31 = 0,
    .a32 = 0,
    .a33 = 1
  };
  return r;
}

struct AMatrix compose(int n, ...) {
  va_list valist;
  va_start(valist, n);
  
  AMatrix r = identity();
  for (int i = 0; i < n; i++) {
    AMatrix x = va_arg(valist, struct AMatrix);
    r = multiply(r,x);
  }
  
  va_end(valist);
  return r;
}

struct AMatrix rotation(double angle) {
  AMatrix r = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a13 = 0,
    .a21 = sin(angle),
    .a22 = cos(angle),
    .a23 = 0,
    .a31 = 0,
    .a32 = 0,
    .a33 = 1
  };
  return r;
}

struct AMatrix translation(struct Vector v) {
  return translation(v.x1, v.x2);
}

struct AMatrix translation(struct AVector v) {
  return translation(v.x1, v.x2);
}

struct AMatrix translation(double x, double y) {
  AMatrix r = {
    .a11 = 1,
    .a12 = 0,
    .a13 = x,
    .a21 = 0,
    .a22 = 1,
    .a23 = y,
    .a31 = 0,
    .a32 = 0,
    .a33 = 1
  };
  return r;
}

struct AMatrix scale(double x_factor, double y_factor) {
  AMatrix r = {
    .a11 = x_factor,
    .a12 = 0,
    .a13 = 0,
    .a21 = 0,
    .a22 = y_factor,
    .a23 = 0,
    .a31 = 0,
    .a32 = 0,
    .a33 = 1
  };
  return r;
}

struct AMatrix shear(double x_shear, double y_shear) {
  AMatrix r = {
    .a11 = 1,
    .a12 = x_shear,
    .a13 = 0,
    .a21 = y_shear,
    .a22 = 1,
    .a23 = 0,
    .a31 = 0,
    .a32 = 0,
    .a33 = 1
  };
  return r;
}

struct Vector multiply(struct Matrix m, struct Vector v) {
  Vector r = {
      .x1 = (m.a11*v.x1) + (m.a12*v.x2),
      .x2 = (m.a21*v.x1) + (m.a22*v.x2)
    };
  return r;
}

struct AVector VectorToAVector(struct Vector v) {
  AVector a = {
    .x1 = v.x1,
    .x2 = v.x2,
    .x3 = 1
  };
  return a;
}

struct Vector AVectorToVector(struct AVector a) {
  Vector v = {
    .x1 = a.x1,
    .x2 = a.x2
  };
  return v;
}

struct Vector vector(double x, double y) {
  Vector v = {
    .x1 = x,
    .x2 = y
  };
  return v;
}

struct Vector multiply(struct AMatrix m, struct Vector v) {
  AVector r = multiply(m, VectorToAVector(v));
  return AVectorToVector(r);
}

struct AVector multiply(struct AMatrix m, struct AVector v) {
  AVector r = {
      .x1 = (m.a11*v.x1) + (m.a12*v.x2) + (m.a13*v.x3),
      .x2 = (m.a21*v.x1) + (m.a22*v.x2) + (m.a23*v.x3),
      .x3 = (m.a31*v.x1) + (m.a32*v.x2) + (m.a33*v.x3)
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

/*** math helper functions ***/

inline double addmod(double x, double mod, double delta) {
  x = x + delta;
  while( x >= mod ) {
    x -= mod;
  }
  while( x <  0.0 ) {
    x += mod;
  }
  return x;
}

inline double addmodpi(double x, double delta) {
  return addmod(x, 2*PI, delta);
}

void testMatrix() {
  Vector v = {.x1 = 0, .x2 = 0};
  AMatrix m = compose( 3,
    rotation(PI/2.0),
    translation(10,0),
    translation(10,0)
  );
  printMatrix(m);
  Vector v1 = multiply(m, v); 
  printVector(v1);
  m = compose( 3,
    translation(10,0),
    rotation(PI/2.0),
    translation(10,0)
  );
  printMatrix(m);
  Vector v2 = multiply(m, v); 
  printVector(v2);
  m = compose( 3,
    translation(10,0),
    translation(10,0),
    rotation(PI/2.0)
  );
  printMatrix(m);
  Vector v3 = multiply(m, v); 
  printVector(v3);
}

