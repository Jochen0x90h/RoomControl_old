#include "ActorValues.hpp"
#include "Ticker.hpp"


void ActorValues::update(const Array<Actor, Scenario::ACTOR_COUNT> & actors) {
	uint32_t time = ticker.getValue();
	
	// elapsed time in milliseconds
	uint32_t d = time - this->time;
	
	// adjust current value
	for (int i = 0; i < Scenario::ACTOR_COUNT; ++i) {
		if (this->running[i]) {
			int speed = actors.get(i).speed;
			int targetValue = this->targetValues[i];
			int &currentValue = this->values[i];
			if (speed == 0) {
				// set immediately
				currentValue = targetValue;
				this->running[i] = false;
			} else {
				int step = d * speed;
				if (currentValue < targetValue) {
					// up
					currentValue += step;
					if (currentValue >= targetValue) {
						currentValue = targetValue;
						this->running[i] = false;
					}
				} else {
					// down
					currentValue -= step;
					if (currentValue <= targetValue) {
						currentValue = targetValue;
						this->running[i] = false;
					}
				}
			}
		}
	}
	
	this->time = time;
}
