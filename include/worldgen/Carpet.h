#pragma once

#include <memory>
#include <random>

#include "Types.h"

namespace Game3 {
	class Realm;

	namespace WorldGen {
		void generateCarpet(const std::shared_ptr<Realm> &, std::default_random_engine &, int padding = -1);
	}
}
