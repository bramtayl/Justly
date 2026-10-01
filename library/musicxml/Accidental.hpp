#pragma once

// how an accidental alters a step
struct Accidental {
  int chromatic = 0;
  // Johnston septimal quartertones (36/35): -1 for a 7, +1 for an el
  int septimal_quartertones = 0;
};
