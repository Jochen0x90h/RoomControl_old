#include "Action.hpp"


bool Actions::deleteId(uint8_t id, uint8_t firstState, uint8_t lastState) {
	int j = 0;
	for (int i = 0; i < this->count; ++i) {
		const Action &action = this->actions[i];
		if (action.id != id && action.state >= firstState && action.state <= lastState) {
			this->actions[j] = action;
			++j;
		}
	}
	if (j < this->count) {
		this->count = j;
		return true;
	}
	return false;
}
