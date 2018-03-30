const int runvar_count = 16;
const double runinc[runvar_count] = { 
  0.0123,   0.00651,  0.000471,  0.000973,   
  0.00223,  0.00751,  0.00879,  0.00443,   
  0.00373,  0.00459,  0.00321,  0.00247,   
  0.00923,  0.00253,  0.00173,  0.00613   
};
double runvar[runvar_count] = { 
  0,        0,        0,        0, 
  0,        0,        0,        0, 
  0,        0,        0,        0, 
  0,        0,        0,        0 
};
double runmod[runvar_count] = { 
  1.0,      2*PI,     2*PI,     2*PI,
  2*PI,     2*PI,     2*PI,     2*PI,
  2*PI,     2*PI,     2*PI,     2*PI,
  2*PI,     2*PI,     2*PI,     2*PI
};

void increment_runvars() {
  for( int i = 0; i < runvar_count; i++ ) {
    runvar[i] = addmod(runvar[i], runmod[i], runinc[i]);
  }
}

void affine_fields() {
  increment_runvars();
  AMatrix m = multiply( translation(sin(runvar[2])*kMatrixWidth, cos(runvar[3])*kMatrixHeight),
              multiply( rotation(sin(runvar[4]) * PI),
              multiply( scale(0.25+sin(runvar[9])/6.0, 0.25+cos(runvar[10])/6.0), 
              multiply( translation(sin(runvar[7])*kMatrixWidth, cos(runvar[8])*kMatrixHeight),
                        rotation(runvar[1])
                        ))));
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector v = multiply(m, vector(x-(kMatrixWidth/2), y-(kMatrixWidth/2)));
      double color = runvar[0] + sinecircle3D(v.x1, v.x2);
      leds[(y*kMatrixWidth) + x] = CHSV( color*255, 255, 255 );
    }
  }
}

