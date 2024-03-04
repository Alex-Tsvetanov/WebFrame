#pragma once

namespace webframe::core {
	enum class LoadingState {
		NOT_STARTED = -1,
		METHOD = 0,
		URI = 1,
		PARAM_KEY = 2,
		PARAM_VALUE = 3,
		HTTP_IN_PROGRESS = 4,
		HTTP_LOADED = 5,
		HEADER_ROW = 6,
		BODY = 7,
		LOADED = 8
	};
}