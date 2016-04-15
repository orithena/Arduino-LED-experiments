
inline boolean between(long l, long x, long u) {
  return (unsigned)(x - l) <= (u - l);
}

int _mod(int x, int m) {
  int r = x%m;
  return r<0 ? r+m : r;
}


