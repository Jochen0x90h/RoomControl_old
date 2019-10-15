#pragma once

#include "Actor.hpp"
#include "Scenario.hpp"
#include "Storage.hpp"
#include "util.hpp"


class ActorValues {
public:

	ActorValues() {
		for (int i = 0; i < Scenario::ACTOR_COUNT; ++i) {
			//this->speed[i] = 0;
			this->targetValues[i] = 0;
			this->values[i] = 0;
			this->running[i] = false;
		}
	}

	//void setSpeed(int actorIndex, int speed) {
	//	this->speed[actorIndex] = speed;
	//}

	/**
	 * Set scenario to actors with a dim level in the range 0 - 100 for each actor
	 * @param scenario array of dim levels, one for each actor
	 */
	void set(const uint8_t *values) {
		for (int i = 0; i < Scenario::ACTOR_COUNT; ++i) {
			int value = values[i];
			if (value <= 100) {
				this->values[i] = clamp(this->values[i], 0, 100 << 16);
				this->targetValues[i] = (value == 0 ? -2 : (value == 100 ? 102 : value)) << 16;
				this->running[i] = true;
			}
		}
	}

	/**
	 * Get the current value of an actor
	 */
	uint8_t get(int index) {
		return clamp(this->values[index] >> 16, 0, 100);
	}

	/**
	 * Stop any moving actor for which the value is valid and return true if at least one actor was stopped
	 */
	bool stop(const uint8_t *values) {
		bool stopped = false;
		for (int i = 0; i < Scenario::ACTOR_COUNT; ++i) {
			if (values[i] <= 100) {
				if (this->running[i]) {
					this->running[i] = false;
					stopped = true;
				}
			}
		}
		return stopped;
	}

	int getSimilarity(const uint8_t *values) {
		int similarity = 0;
		for (int i = 0; i < Scenario::ACTOR_COUNT; ++i) {
			if (values[i] <= 100) {
				similarity += abs(clamp(this->targetValues[i] >> 16, 0, 100) - values[i]);
			}
		}
		return similarity;
	}

	void update(const Array<Actor, Scenario::ACTOR_COUNT> & actors);
	
	int getSwitchState(int index) {
		return this->values[index] >= (50 << 16) ? 1 : 0;
	}
	
	int getBlindState(int index) {
		if (this->running[index]) {
			int targetValue = this->targetValues[index];
			int currentValue = this->values[index];
			if (currentValue < targetValue) {
				// up
				return 2;
			} else if (currentValue > targetValue) {
				// down
				return 1;
			}
		}
		return 0;
	}

protected:

	// target states of actors
	int targetValues[Scenario::ACTOR_COUNT];
	
	// current states of actors (simulated, not measured)
	int values[Scenario::ACTOR_COUNT];

	bool running[Scenario::ACTOR_COUNT];
	
	// last time for determining a time step
	uint32_t time = 0;
};
