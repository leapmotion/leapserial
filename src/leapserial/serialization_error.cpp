#include "stdafx.h"
#include "serialization_error.h"

leap::serialization_error::serialization_error(std::string what) :
  std::runtime_error(std::move(what))
{}
