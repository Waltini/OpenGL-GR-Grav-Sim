#pragma once

#ifndef STEPRK4_H_INCLUDED
#define STEPRK4_H_INCLUDED

#include "celestial_body_class.h"

void step(celestial_body& b1, celestial_body& b2, long double& dt, bool years);

#endif
