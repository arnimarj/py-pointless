#!/bin/bash

(cd judy-1.0.5/src; COPT='-DJU_64BIT -O0 -fPIC -fno-strict-aliasing -fno-aggressive-loop-optimizations' sh ./sh_build)
